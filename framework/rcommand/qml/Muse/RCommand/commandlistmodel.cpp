/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) MuseScore/Audacity and others
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
 #include "commandlistmodel.h"

 #include "log.h"
#include <algorithm>

using namespace muse::rcommand;

CommandListModel::CommandListModel(QObject* parent)
    : QAbstractListModel(parent), Contextable(muse::iocCtxForQmlObject(this))
{
}

void CommandListModel::classBegin()
{
    const std::vector<CommandInfo>& infos = commandsRegister()->commandInfoList();

    beginResetModel();

    for (const CommandInfo& info : infos) {
        Item item;
        item.info = info;
        item.state = commandsState()->commandState(item.info.command);

        formatItem(item);

        m_allItems.append(item);
    }

    std::sort(m_allItems.begin(), m_allItems.end(), [](const Item& f, const Item& s) {
        return f.info.command < s.info.command;
    });

    m_items = m_allItems;

    endResetModel();

    commandsState()->commandStateChanged().onReceive(this, [this](const Command& command, const CommandState& state) {
        updateState(command, state);
    });
}

void CommandListModel::formatItem(Item& item)
{
    item.formatted = QString("%1\ntitle: %2\ndescription: %3\nenabled: %4, checked: %5")
                     .arg(item.info.command.toString())
                     .arg(item.info.title.qTranslatedWithoutMnemonic())
                     .arg(item.info.description.qTranslated())
                     .arg(item.state.enabled)
                     .arg(item.state.checked);
}

void CommandListModel::updateState(const Command& command, const CommandState& state)
{
    // update state in m_allItems

    auto ait = std::find_if(m_allItems.begin(), m_allItems.end(), [&command](const Item& item) {
        return item.info.command == command;
    });

    if (ait != m_allItems.end()) {
        ait->state = state;
        formatItem(*ait);
    }

    // update state in m_items (current visible items)
    auto cit = std::find_if(m_items.begin(), m_items.end(), [&command](const Item& item) {
        return item.info.command == command;
    });

    if (cit != m_items.end()) {
        int idx = (int)std::distance(m_items.begin(), cit);
        *cit = *ait;
        emit dataChanged(index(idx), index(idx), { rItemData });
    }
}

QVariant CommandListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
    case rItemData: return QString::number(index.row()) + ". " + m_items.at(index.row()).formatted;
    }

    return QVariant();
}

int CommandListModel::rowCount(const QModelIndex&) const
{
    return m_items.count();
}

QHash<int, QByteArray> CommandListModel::roleNames() const
{
    return { { rItemData, "itemData" } };
}

void CommandListModel::dispatch(int index)
{
    const Item& item = m_items.at(index);
    commandDispatcher()->dispatch(item.info.command);
    LOGD() << "dispatch: " << item.info.command.toString();
}

void CommandListModel::find(const QString& str)
{
    beginResetModel();

    m_searchText = str;

    if (m_searchText.isEmpty()) {
        m_items = m_allItems;
    } else {
        m_items.clear();
        for (const Item& item : m_allItems) {
            if (item.formatted.contains(m_searchText, Qt::CaseInsensitive)) {
                m_items.append(item);
            }
        }
    }

    //LOGD() << "searchText: " << m_searchText << ", items: " << m_items.count();

    endResetModel();
}

void CommandListModel::print()
{
    for (const Item& item : m_items) {
        std::cout << item.formatted.toStdString() << std::endl;
    }
}
