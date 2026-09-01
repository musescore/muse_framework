/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited
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
#include "toastlistmodel.h"

#include <algorithm>

#include <QModelIndex>
#include <QString>

#include "toast/toastitem.h"

using namespace muse::toast;

namespace {
constexpr int MAX_VISIBLE_TOASTS = 5;
}

ToastListModel::ToastListModel(QObject* parent)
    : QAbstractListModel(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void ToastListModel::init()
{
    toastProvider()->toastAdded().onReceive(this, [this](std::shared_ptr<ToastItem> toast) {
        if (m_toasts.size() >= MAX_VISIBLE_TOASTS) {
            toastProvider()->dismissToast(m_toasts.front()->id());
        }

        beginInsertRows(QModelIndex(), static_cast<int>(m_toasts.size()), static_cast<int>(m_toasts.size()));
        m_toasts.emplace_back(toast);
        endInsertRows();

        int id = toast->id();
        toast->progressChanged().onNotify(this, [this, id](){
            const std::optional<int> toastIndex = indexOfToast(id);
            if (!toastIndex) {
                return;
            }

            emit dataChanged(index(toastIndex.value()), this->index(toastIndex.value()), { ProgressRole, TimeElapsedRole });
        });
    }, muse::async::Asyncable::Mode::SetReplace);

    toastProvider()->toastDismissed().onReceive(this, [this](int id) {
        const std::optional<int> toastIndex = indexOfToast(id);
        if (!toastIndex) {
            return;
        }

        beginRemoveRows(QModelIndex(), toastIndex.value(), toastIndex.value());
        m_toasts.erase(m_toasts.begin() + toastIndex.value());
        endRemoveRows();
    }, muse::async::Asyncable::Mode::SetReplace);
}

std::optional<int> ToastListModel::indexOfToast(int id) const
{
    const auto it = std::find_if(m_toasts.cbegin(), m_toasts.cend(), [id](const std::shared_ptr<ToastItem>& toast) {
        return toast->id() == id;
    });

    if (it == m_toasts.cend()) {
        return std::nullopt;
    }

    return static_cast<int>(std::distance(m_toasts.cbegin(), it));
}

void ToastListModel::dismissToast(int id)
{
    toastProvider()->dismissToast(id);
}

void ToastListModel::executeAction(int id, QString actionStr)
{
    const std::optional<int> toastIndex = indexOfToast(id);
    if (!toastIndex) {
        return;
    }

    const auto& actions = m_toasts.at(toastIndex.value())->actions();
    auto it = std::find_if(actions.cbegin(), actions.cend(), [&actionStr](const ToastAction& action) {
        return action.text == actionStr.toStdString();
    });

    if (it == actions.cend()) {
        return;
    }

    toastProvider()->executeAction(id, it->code);
}

int ToastListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(m_toasts.size());
}

QVariant ToastListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_toasts.size())) {
        return QVariant();
    }

    const auto toast = m_toasts.at(index.row());
    switch (role) {
    case IdRole:
        return toast->id();
    case IconCodeRole:
        return static_cast<int>(toast->iconCode());
    case TitleRole:
        return QString::fromStdString(toast->title());
    case MessageRole:
        return QString::fromStdString(toast->message());
    case DismissableRole:
        return toast->isDismissible();
    case ActionRole:
    {
        QVariantList actionsList;
        for (auto& action : toast->actions()) {
            QVariantMap actionMap;
            actionMap.insert("text", QString::fromStdString(action.text));
            actionsList.append(actionMap);
        }
        return actionsList;
    }
    case ProgressRole:
        return static_cast<int>(toast->currentProgress());
    case ShowProgressInfoRole:
        return toast->showProgressInfo();
    case TimeElapsedRole:
        return toast->timeElapsed();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ToastListModel::roleNames() const
{
    static QHash<int, QByteArray> roles {
        { IdRole, "id" },
        { IconCodeRole, "iconCode" },
        { TitleRole, "title" },
        { MessageRole, "message" },
        { DismissableRole, "dismissable" },
        { ActionRole, "actions" },
        { ProgressRole, "progress" },
        { TimeElapsedRole, "timeElapsed" },
        { ShowProgressInfoRole, "showProgressInfo" },
    };
    return roles;
}
