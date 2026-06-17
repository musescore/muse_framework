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

#pragma once

#include <QAbstractListModel>
#include <QQmlParserStatus>

#include "global/async/asyncable.h"

#include "commandtypes.h"
#include "modularity/ioc.h"
#include "icommandsregister.h"
#include "icommandsstate.h"
#include "icommanddispatcher.h"

namespace muse::rcommand {
class CommandListModel : public QAbstractListModel, public QQmlParserStatus, public Contextable, public async::Asyncable
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    QML_ELEMENT

    GlobalInject<ICommandsRegister> commandsRegister;
    ContextInject<ICommandsState> commandsState = { this };
    ContextInject<ICommandDispatcher> commandDispatcher = { this };

public:
    explicit CommandListModel(QObject* parent = nullptr);

    Q_INVOKABLE void dispatch(int index);
    Q_INVOKABLE void find(const QString& str);
    Q_INVOKABLE void print();

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    enum Roles {
        rItemData = Qt::UserRole + 1
    };

    struct Item {
        CommandInfo info;
        CommandState state;

        QString formatted;
    };

    void classBegin() override;
    void componentComplete() override {}

    void formatItem(Item& item);
    void updateState(const Command& command, const CommandState& state);

    QList<Item> m_allItems;
    QList<Item> m_items;

    QString m_searchText;
};
}
