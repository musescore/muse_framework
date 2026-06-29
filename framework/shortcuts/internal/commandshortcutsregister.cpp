/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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
#include "commandshortcutsregister.h"

#include "global/io/file.h"
#include "global/serialization/json.h"

using namespace muse;
using namespace muse::shortcuts;
using namespace muse::async;

static const std::string COMMAND_SHORTCUTS_TAG("CommandShortcuts");

static const std::map<QKeySequence::StandardKey, QKeyCombination> COMMAND_SHORTCUTS_EXPAND_IGNORE_MAP = {
    { QKeySequence::StandardKey::HelpContents, Qt::Key_Help },
    { QKeySequence::StandardKey::Open, Qt::Key_Open },
    { QKeySequence::StandardKey::Close, Qt::Key_Close },
    { QKeySequence::StandardKey::Save, Qt::Key_Save },
    { QKeySequence::StandardKey::New, Qt::Key_New },
    { QKeySequence::StandardKey::Cut, Qt::Key_Cut },
    { QKeySequence::StandardKey::Copy, Qt::Key_Copy },
    { QKeySequence::StandardKey::Paste, Qt::Key_Paste },
    { QKeySequence::StandardKey::Undo, Qt::Key_Undo },
    { QKeySequence::StandardKey::Redo, Qt::Key_Redo },
    { QKeySequence::StandardKey::Forward, Qt::Key_Forward },
    { QKeySequence::StandardKey::Refresh, Qt::Key_Refresh },
    { QKeySequence::StandardKey::ZoomIn, Qt::Key_ZoomIn },
    { QKeySequence::StandardKey::ZoomOut, Qt::Key_ZoomOut },
    { QKeySequence::StandardKey::Find, Qt::Key_Find },
    { QKeySequence::StandardKey::SaveAs, (Qt::SHIFT | Qt::Key_Save) },
    { QKeySequence::StandardKey::Preferences, Qt::Key_Settings },
    { QKeySequence::StandardKey::Quit, Qt::Key_Exit },
    { QKeySequence::StandardKey::Cancel, Qt::Key_Cancel }
};

void CommandShortcutsRegister::init()
{
    multiwindowsProvider()->resourceChanged().onReceive(this, [this](const std::string& resourceName) {
        if (resourceName == COMMAND_SHORTCUTS_TAG) {
            reload();
        }
    });

    reload();
}

void CommandShortcutsRegister::reload(bool onlyDef)
{
    TRACEFUNC;

    m_shortcuts.clear();
    m_defaultShortcuts.clear();

    io::path_t defPath = configuration()->commandShortcutsAppDataPath();
    io::path_t userPath = configuration()->commandShortcutsUserAppDataPath();

    bool ok = readFromFile(m_defaultShortcuts, defPath);

    if (ok) {
        expandStandardKeys(m_defaultShortcuts);

        if (!onlyDef) {
            if (!io::File::exists(userPath)) {
                ok = false;
            } else {
                //! NOTE The user shortcut file may change, so we need to lock it
                mi::ReadResourceLockGuard guard(multiwindowsProvider(), COMMAND_SHORTCUTS_TAG);
                ok = readFromFile(m_shortcuts, userPath);
            }
        } else {
            ok = false;
        }

        if (!ok) {
            m_shortcuts = m_defaultShortcuts;
        } else {
            mergeShortcuts(m_shortcuts, m_defaultShortcuts);
            mergeAdditionalShortcuts(m_shortcuts);
        }

        ok = true;
    }

    if (ok) {
        expandStandardKeys(m_shortcuts);
        makeUnique(m_shortcuts);
        m_shortcutsChanged.notify();
    }
}

void CommandShortcutsRegister::mergeShortcuts(ShortcutList& shortcuts, const ShortcutList& defaultShortcuts) const
{
    TRACEFUNC;

    ShortcutList needadd;
    for (const Shortcut& defSc : defaultShortcuts) {
        Shortcut scForAdd = defSc;
        bool found = false;

        for (Shortcut& sc : shortcuts) {
            //! NOTE If a user shortcut is found, set scope & auto repeat (always use default values)
            if (sc.command == defSc.command) {
                sc.scope = defSc.scope;
                sc.autoRepeat = defSc.autoRepeat;
                found = true;
            } else if (sc.scope == defSc.scope) {
                for (const std::string& seq : sc.sequences) {
                    //! NOTE If user shortcut has sequence from default shortcut, remove the sequence from default shortcut
                    muse::remove_if(scForAdd.sequences, [&seq](const std::string& cmp){
                        return cmp == seq;
                    });
                }
            }
        }

        //! NOTE If no default shortcut is found in user shortcuts add def
        if (!found) {
            needadd.push_back(scForAdd);
        }
    }

    if (!needadd.empty()) {
        shortcuts.splice(shortcuts.end(), needadd);
    }
}

void CommandShortcutsRegister::mergeAdditionalShortcuts(ShortcutList& shortcuts)
{
    for (const auto& [context, additionalShortcuts] : m_additionalShortcutsMap) {
        mergeShortcuts(shortcuts, additionalShortcuts);
    }
}

void CommandShortcutsRegister::makeUnique(ShortcutList& shortcuts)
{
    TRACEFUNC;

    const ShortcutList all = shortcuts;

    shortcuts.clear();

    for (const Shortcut& sc : all) {
        const std::string& command = sc.command;

        auto it = std::find_if(shortcuts.begin(), shortcuts.end(), [command](const Shortcut& s) {
            return s.command == command;
        });

        if (it == shortcuts.end()) {
            shortcuts.push_back(sc);
            continue;
        }

        Shortcut& foundSc = *it;

        IF_ASSERT_FAILED(foundSc.scope == sc.scope) {
        }

        foundSc.sequences.insert(foundSc.sequences.end(), sc.sequences.begin(), sc.sequences.end());
    }
}

void CommandShortcutsRegister::expandStandardKeys(ShortcutList& shortcuts) const
{
    TRACEFUNC;

    ShortcutList expanded;
    ShortcutList notbonded;

    for (Shortcut& shortcut : shortcuts) {
        QKeySequence ignoredSeq = QKeySequence(muse::value(COMMAND_SHORTCUTS_EXPAND_IGNORE_MAP, shortcut.standardKey, Qt::Key_unknown));

        if (!shortcut.sequences.empty()) {
            std::string ignoredSeqStr = ignoredSeq.toString().toStdString();
            muse::remove_if(shortcut.sequences, [&ignoredSeqStr](const std::string& seq) {
                return seq == ignoredSeqStr;
            });

            continue;
        }

        QList<QKeySequence> kslist = QKeySequence::keyBindings(shortcut.standardKey);
        if (kslist.isEmpty()) {
            notbonded.push_back(shortcut);
            continue;
        }

        const QKeySequence& first = kslist.first();
        shortcut.sequences.push_back(first.toString().toStdString());
        //LOGD() << "for standard key: " << sc.standardKey << ", sequence: " << sc.sequence;

        //! NOTE If the keyBindings contains more than one result,
        //! these can be considered alternative shortcuts on the same platform for the given key.
        for (int i = 1; i < kslist.count(); ++i) {
            const QKeySequence& seq = kslist.at(i);
            if (seq == ignoredSeq) {
                continue;
            }

            Shortcut esc = shortcut;
            esc.sequences = { seq.toString().toStdString() };
            //LOGD() << "for standard key: " << esc.standardKey << ", alternative sequence: " << esc.sequence;
            expanded.push_back(esc);
        }
    }

    if (!notbonded.empty()) {
        LOGD() << "removed " << notbonded.size() << " shortcut, because they are not bound to standard key";
        for (const Shortcut& sc : notbonded) {
            shortcuts.remove(sc);
        }
    }

    if (!expanded.empty()) {
        LOGD() << "added " << expanded.size() << " shortcut, because they are alternative shortcuts for the given standard keys";

        shortcuts.splice(shortcuts.end(), expanded);
    }
}

ShortcutList CommandShortcutsRegister::filterAndUpdateAdditionalShortcuts(const ShortcutList& shortcuts)
{
    ShortcutList noAdditionalShortcuts = shortcuts;

    for (auto& [context, additionalShortcuts] : m_additionalShortcutsMap) {
        for (Shortcut& shortcut : additionalShortcuts) {
            auto it = std::find(shortcuts.begin(), shortcuts.end(), shortcut.action);
            if (it != shortcuts.end()) {
                shortcut = *it;
                noAdditionalShortcuts.remove(shortcut);
            }
        }
    }

    return noAdditionalShortcuts;
}

bool CommandShortcutsRegister::readFromFile(ShortcutList& shortcuts, const io::path_t& path) const
{
    TRACEFUNC;

    ByteArray data;
    Ret ret = io::File::readFile(path, data);
    if (!ret) {
        LOGE() << "failed read file: " << path << ", err: " << ret.toString();
        return false;
    }

    JsonDocument doc = JsonDocument::fromJson(data);
    if (!doc.isObject()) {
        LOGE() << "failed parse json: " << path;
        return false;
    }

    JsonObject obj = doc.rootObject();

    for (const std::string& key : obj.keys()) {
        JsonValue scope = obj.value(key);
        if (!scope.isArray()) {
            LOGE() << "failed parse json: " << path << ", key: " << key;
            continue;
        }

        JsonArray arr = scope.toArray();
        for (size_t i = 0; i < arr.size(); ++i) {
            JsonValue value = arr.at(i);
            if (!value.isObject()) {
                LOGE() << "failed parse json: " << path << ", key: " << key << ", i: " << i;
                continue;
            }

            JsonObject obj = value.toObject();
            Shortcut shortcut;
            shortcut.scope = key;
            shortcut.command = obj.value("command").toString().toStdString();
            shortcut.autoRepeat = obj.value("autoRepeat").toBool();

            JsonValue sequences = obj.value("sequences");
            if (!sequences.isArray()) {
                LOGE() << "failed parse json: " << path << ", key: " << key << ", i: " << i;
                continue;
            }

            JsonArray sequencesArr = sequences.toArray();
            for (size_t j = 0; j < sequencesArr.size(); ++j) {
                JsonValue sequence = sequencesArr.at(j);
                if (!sequence.isString()) {
                    LOGE() << "failed parse json: " << path << ", key: " << key << ", i: " << i << ", j: " << j;
                    continue;
                }
                shortcut.sequences.push_back(sequence.toString().toStdString());
            }

            shortcuts.push_back(shortcut);
        }
    }

    return true;
}

bool CommandShortcutsRegister::writeToFile(const ShortcutList& shortcuts, const io::path_t& path) const
{
    TRACEFUNC;

    JsonObject root;
    for (const Shortcut& shortcut : shortcuts) {
        JsonObject shortcutObj;
        shortcutObj["command"] = shortcut.command;
        shortcutObj["autoRepeat"] = shortcut.autoRepeat;
        JsonArray sequencesArr;
        for (const std::string& sequence : shortcut.sequences) {
            sequencesArr.append(JsonValue(sequence));
        }
        shortcutObj["sequences"] = sequencesArr;

        JsonValue scopeValue = root.value(shortcut.scope);
        if (!scopeValue.isArray()) {
            scopeValue = JsonArray();
        }
        JsonArray scopeValueArr = scopeValue.toArray();
        scopeValueArr.append(shortcutObj);
        root[shortcut.scope] = scopeValueArr;
    }

    ByteArray data = JsonDocument(root).toJson();

    Ret ret;
    {
        mi::WriteResourceLockGuard guard(multiwindowsProvider(), COMMAND_SHORTCUTS_TAG);
        ret = io::File::writeFile(path, data);
    }

    LOGD() << "write shortcuts to file: " << path;

    if (!ret) {
        LOGE() << ret.toString();
    }

    return ret;
}

const ShortcutList& CommandShortcutsRegister::shortcuts() const
{
    return m_shortcuts;
}

Ret CommandShortcutsRegister::setShortcuts(const ShortcutList& shortcuts)
{
    TRACEFUNC;

    if (shortcuts == m_shortcuts) {
        return true;
    }

    ShortcutList needToWrite = filterAndUpdateAdditionalShortcuts(shortcuts);

    bool ok = writeToFile(needToWrite, configuration()->commandShortcutsUserAppDataPath());

    if (ok) {
        m_shortcuts = needToWrite;
        mergeShortcuts(m_shortcuts, m_defaultShortcuts);
        mergeAdditionalShortcuts(m_shortcuts);
        m_shortcutsChanged.notify();
    }

    return ok;
}

void CommandShortcutsRegister::resetShortcuts()
{
    {
        mi::WriteResourceLockGuard guard(multiwindowsProvider(), COMMAND_SHORTCUTS_TAG);
        io::File::remove(configuration()->commandShortcutsUserAppDataPath());
    }

    reload();
}

Notification CommandShortcutsRegister::shortcutsChanged() const
{
    return m_shortcutsChanged;
}

Ret CommandShortcutsRegister::setAdditionalShortcuts(const std::string& context, const ShortcutList& shortcuts)
{
    m_additionalShortcutsMap[context] = shortcuts;

    mergeShortcuts(m_shortcuts, m_additionalShortcutsMap[context]);
    m_shortcutsChanged.notify();

    return muse::make_ok();
}

ShortcutList CommandShortcutsRegister::shortcutsForSequence(const std::string& sequence) const
{
    ShortcutList list;
    for (const Shortcut& sh : m_shortcuts) {
        auto it = std::find(sh.sequences.cbegin(), sh.sequences.cend(), sequence);
        if (it != sh.sequences.cend()) {
            list.push_back(sh);
        }
    }
    return list;
}

Ret CommandShortcutsRegister::importFromFile(const io::path_t& filePath)
{
    {
        mi::ReadResourceLockGuard guard(multiwindowsProvider(), COMMAND_SHORTCUTS_TAG);
        Ret ret = io::File::copy(filePath, configuration()->commandShortcutsUserAppDataPath(), true);
        if (!ret) {
            LOGE() << "failed import file: " << ret.toString();
            return ret;
        }
    }

    reload();

    return make_ret(Ret::Code::Ok);
}

Ret CommandShortcutsRegister::exportToFile(const io::path_t& filePath) const
{
    return writeToFile(m_shortcuts, filePath);
}
