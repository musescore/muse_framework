/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "shortcutsmodel.h"

#include "translation.h"
#include "types/mnemonicstring.h"
#include "types/translatablestring.h"
#include "shortcutcontext.h"

#include "log.h"

using namespace muse::shortcuts;
using namespace muse::ui;
using namespace muse::rcommand;

static std::vector<std::string> shortcutsFileFilter()
{
    return { muse::trc("shortcuts", "MuseScore Studio shortcuts file") + " (*.xml)" };
}

ShortcutsModel::ShortcutsModel(QObject* parent)
    : QAbstractListModel(parent), Contextable(muse::iocCtxForQmlObject(this))
{
}

QVariant ShortcutsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const Item& item = m_items.at(index.row());

    switch (role) {
    case RoleTitle: return item.title;
    case RoleIcon: return item.icon;
    case RoleSequence: return item.sequence;
    case RoleSearchKey: return item.searchKey + item.sequence;
    }

    return QVariant();
}

const UiAction& ShortcutsModel::action(const std::string& actionCode) const
{
    return uiactionsRegister()->action(actionCode);
}

QString ShortcutsModel::actionText(const std::string& actionCode) const
{
    const UiAction& action = this->action(actionCode);

    if (action.description.isEmpty()) {
        return action.title.qTranslatedWithoutMnemonic();
    }

    return action.description.qTranslated();
}

int ShortcutsModel::rowCount(const QModelIndex&) const
{
    return m_items.size();
}

QHash<int, QByteArray> ShortcutsModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        { RoleTitle, "title" },
        { RoleIcon, "icon" },
        { RoleSequence, "sequence" },
        { RoleSearchKey, "searchKey" }
    };

    return roles;
}

void ShortcutsModel::load()
{
    beginResetModel();
    m_items.clear();

    // command shortcuts
    {
        const std::vector<CommandInfo>& commands = commandsRegister()->commandInfoList();
        for (const Shortcut& shortcut : commandShortcutsRegister()->shortcuts()) {
            const Command& command = Command(shortcut.command);
            auto it = std::find_if(commands.begin(), commands.end(), [command](const CommandInfo& info) {
                return info.command == command;
            });

            if (it == commands.end()) {
                LOGD() << "Command not found: " << shortcut.command;
                continue;
            }

            const CommandInfo& info = *it;
            Item item;
            item.shortcut = shortcut;

            item.group = QString::fromStdString(shortcut.scope);

            if (info.description.isEmpty()) {
                item.title = info.title.qTranslatedWithoutMnemonic();
            } else {
                item.title = info.description.qTranslated();
            }

            item.icon = static_cast<int>(info.decoration.iconCode);
            item.sequence = sequencesToNativeText(shortcut.sequences);
            item.searchKey = QString::fromStdString(info.command.toString())
                             + info.title.qTranslatedWithoutMnemonic()
                             + info.description.qTranslated();

            m_items.append(item);
        }

        commandShortcutsRegister()->shortcutsChanged().onNotify(this, [this]() {
            load();
        }, async::Asyncable::Mode::SetReplace);
    }

    // actions shortcuts
    {
        for (const UiAction& action : uiactionsRegister()->actionList()) {
            if (action.scCtx == CTX_DISABLED) {
                continue;
            }

            Shortcut shortcut = shortcutsRegister()->shortcut(action.code);
            if (!shortcut.isValid()) {
                shortcut.action = action.code;
                shortcut.context = action.scCtx;
            }

            Item item;
            item.shortcut = shortcut;
            item.title = actionText(action.code);
            item.icon = static_cast<int>(action.iconCode);
            item.sequence = sequencesToNativeText(shortcut.sequences);
            item.searchKey = QString::fromStdString(action.code)
                             + action.title.qTranslatedWithoutMnemonic()
                             + action.description.qTranslated();

            m_items.append(item);
        }

        shortcutsRegister()->shortcutsChanged().onNotify(this, [this]() {
            load();
        }, async::Asyncable::Mode::SetReplace);
    }

    std::sort(m_items.begin(), m_items.end(), [](const Item& i1, const Item& i2) {
        return i1.group > i2.group || (i1.group == i2.group && i1.title < i2.title);
    });

    endResetModel();
}

bool ShortcutsModel::apply()
{
    // command shortcuts
    {
        ShortcutList shortcuts;
        for (const Item& item : std::as_const(m_items)) {
            if (item.shortcut.command.empty()) {
                continue;
            }
            shortcuts.push_back(item.shortcut);
        }
        Ret ret = commandShortcutsRegister()->setShortcuts(shortcuts);
        if (!ret) {
            LOGE() << ret.toString();
            return false;
        }
    }

    {
        ShortcutList shortcuts;
        for (const Item& item : std::as_const(m_items)) {
            if (item.shortcut.action.empty()) {
                continue;
            }
            shortcuts.push_back(item.shortcut);
        }
        Ret ret = shortcutsRegister()->setShortcuts(shortcuts);
        if (!ret) {
            LOGE() << ret.toString();
            return false;
        }
    }

    return true;
}

void ShortcutsModel::reset()
{
    commandShortcutsRegister()->resetShortcuts();
    shortcutsRegister()->resetShortcuts();
}

QItemSelection ShortcutsModel::selection() const
{
    return m_selection;
}

QVariant ShortcutsModel::currentShortcut() const
{
    QModelIndex index = currentShortcutIndex();
    if (!index.isValid()) {
        return QVariant();
    }

    const Item& item = m_items.at(index.row());
    return shortcutToObject(item);
}

QModelIndex ShortcutsModel::currentShortcutIndex() const
{
    if (m_selection.size() == 1) {
        return m_selection.indexes().first();
    }

    return QModelIndex();
}

void ShortcutsModel::setSelection(const QItemSelection& selection)
{
    if (m_selection == selection) {
        return;
    }

    m_selection = selection;
    emit selectionChanged();
}

void ShortcutsModel::importShortcutsFromFile()
{
    io::path_t path = interactive()->selectOpeningFileSync(
        muse::trc("shortcuts", "Import shortcuts"),
        globalConfiguration()->homePath(),
        shortcutsFileFilter());

    if (!path.empty()) {
        shortcutsRegister()->importFromFile(path);
    }
}

void ShortcutsModel::exportShortcutsToFile()
{
    io::path_t path = interactive()->selectSavingFileSync(
        muse::trc("shortcuts", "Export shortcuts"),
        globalConfiguration()->homePath(),
        shortcutsFileFilter());

    if (path.empty()) {
        return;
    }

    Ret ret = shortcutsRegister()->exportToFile(path);
    if (!ret) {
        LOGE() << ret.toString();
    }
}

void ShortcutsModel::applySequenceToCurrentShortcut(const QString& newSequence, int conflictShortcutIndex)
{
    QModelIndex currIndex = currentShortcutIndex();
    if (!currIndex.isValid()) {
        return;
    }

    int row = currIndex.row();
    m_items[row].shortcut.sequences = Shortcut::sequencesFromString(newSequence.toStdString());
    m_items[row].sequence = sequencesToNativeText(m_items[row].shortcut.sequences);
    LOGD() << "apply sequence to command: " << m_items[row].shortcut.command << " new sequence: " << newSequence.toStdString();

    if (conflictShortcutIndex >= 0 && conflictShortcutIndex < m_items.size()) {
        m_items[conflictShortcutIndex].shortcut.clear();
        m_items[conflictShortcutIndex].sequence = "";
        LOGD() << "clear sequence for command: " << m_items[conflictShortcutIndex].shortcut.command;
        notifyAboutShortcutChanged(index(conflictShortcutIndex));
    }

    notifyAboutShortcutChanged(currIndex);
}

void ShortcutsModel::clearSelectedShortcuts()
{
    for (const QModelIndex& index : m_selection.indexes()) {
        Item& item = m_items[index.row()];
        item.shortcut.clear();
        item.sequence = "";
        notifyAboutShortcutChanged(index);
    }
}

void ShortcutsModel::notifyAboutShortcutChanged(const QModelIndex& index)
{
    emit dataChanged(index, index);
}

void ShortcutsModel::resetToDefaultSelectedShortcuts()
{
    auto resolveConflicts = [this](const Shortcut& shortcut) {
        for (int i = 0; i < m_items.size(); ++i) {
            Shortcut& sc = m_items[i].shortcut;

            if (shortcut == sc) {
                continue;
            }

            if (!areContextPrioritiesEqual(shortcut.context, sc.context)) {
                continue;
            }

            if (shortcut.sequences == sc.sequences) {
                sc.clear();
                notifyAboutShortcutChanged(index(i));
            }
        }
    };

    for (const QModelIndex& index : m_selection.indexes()) {
        Shortcut& shortcut = m_items[index.row()].shortcut;

        const Shortcut& defaultShortcut = shortcutsRegister()->defaultShortcut(shortcut.action);
        if (defaultShortcut.isValid()) {
            shortcut = defaultShortcut;
        } else {
            shortcut.sequences = {};
        }

        resolveConflicts(shortcut);

        notifyAboutShortcutChanged(index);
    }
}

QVariantList ShortcutsModel::shortcuts() const
{
    QVariantList result;

    for (const Item& item : std::as_const(m_items)) {
        result << shortcutToObject(item);
    }

    return result;
}

QVariant ShortcutsModel::shortcutToObject(const Item& item) const
{
    QVariantMap obj;
    obj["title"] = item.title;
    obj["sequence"] = item.sequence;
    obj["context"] = QString::fromStdString(item.shortcut.context);
    obj["autoRepeat"] = item.shortcut.autoRepeat;

    return obj;
}
