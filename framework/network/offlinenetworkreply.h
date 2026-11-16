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
#pragma once

#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QUrl>

namespace muse::network {
//! NOTE Local urls (qrc, file, data) are not a network access, they work in offline mode
inline bool isNetworkUrl(const QUrl& url)
{
    const QString scheme = url.scheme().toLower();
    return scheme == "http" || scheme == "https" || scheme == "ftp" || scheme == "ws" || scheme == "wss";
}

//! NOTE The reply to a request in offline mode: fails as a network error,
//! asynchronously as a real request does
class OfflineNetworkReply : public QNetworkReply
{
public:
    OfflineNetworkReply(QNetworkAccessManager::Operation operation, const QNetworkRequest& request, QObject* parent = nullptr)
        : QNetworkReply(parent)
    {
        setOperation(operation);
        setRequest(request);
        setUrl(request.url());
        setOpenMode(QIODevice::ReadOnly);
        setError(QNetworkReply::UnknownNetworkError, QStringLiteral("Network access is disabled (offline mode)"));

        QTimer::singleShot(0, this, [this]() {
            setFinished(true);
            emit errorOccurred(error());
            emit finished();
        });
    }

    void abort() override {}

protected:
    qint64 readData(char*, qint64) override { return -1; }
};
}
