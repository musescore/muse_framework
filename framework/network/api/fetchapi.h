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

#include <QBuffer>
#include <QVariantMap>
#include <qstringview.h>

#include "global/api/apiobject.h"
#include "global/async/asyncable.h"

#include "modularity/ioc.h"
#include "network/inetworkconfiguration.h"
#include "network/inetworkmanager.h"
#include "network/inetworkmanagercreator.h"

namespace muse::network::api {
class FetchApi : public muse::api::ApiObject, public muse::async::Asyncable
{
    Q_OBJECT

    GlobalInject<INetworkConfiguration> configuration;
    GlobalInject<INetworkManagerCreator> networkManagerCreator;

public:
    explicit FetchApi(muse::api::IApiEngine* e);

    Q_INVOKABLE QJSValue fetch(QJSValue request, QJSValue options = {});
    Q_INVOKABLE QJSValue toUtf8(const QJSValue& bytes) const;

private:

    QString stringOption(const QJSValue& options, const QString& key, const QString& def) const;
    QByteArray byteArrayOption(const QJSValue& options, const QString& key, const QByteArray& def) const;
    RequestHeaders requestHeaders(const QJSValue& options) const;
    RetVal<Progress> execRequest(const QString& method, const QUrl& url, QIODevicePtr outgoing, QIODevicePtr incoming,
                                 const RequestHeaders& headers);

    QJSValue responseFactory();
    QJSValue makeResponse(int status, const QString& statusText, const QByteArray& body, const QVariantMap& headers);

    INetworkManagerPtr m_networkManager;
    QJSValue m_responseFactory;
    QJSValue m_self;
};
}
