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

#include "fetchapi.h"

#include <QBuffer>
#include <QJSEngine>
#include <QJSValueIterator>
#include <QUrl>
#include <QVariantMap>

#include "global/progress.h"

#include "log.h"

using namespace muse;
using namespace muse::network;
using namespace muse::network::api;

FetchApi::FetchApi(muse::api::IApiEngine* e)
    : muse::api::ApiObject(e)
{
    m_self = engine()->newQObject(this);
}

QJSValue FetchApi::fetch(QJSValue request, QJSValue options)
{
    QUrl url;
    if (request.isString()) {
        url = QUrl(request.toString());
    } else {
        NOT_SUPPORTED;
    }

    muse::api::JsPromise promise = engine()->newPromise();

    if (!url.isValid() || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        promise.reject.call({ QJSValue(QStringLiteral("Invalid url")) });
        return promise.promise;
    }

    const QString method = stringOption(options, "method", "GET").toUpper();
    LOGDA() << "fetch:" << method << " " << url;

    const RequestHeaders headers = requestHeaders(options);
    const QByteArray body = byteArrayOption(options, "body", QByteArray());

    auto outgoing = std::make_shared<QBuffer>();
    outgoing->setData(body);

    auto incoming = std::make_shared<QBuffer>();
    RetVal<Progress> progress = execRequest(method, url, outgoing, incoming, headers);
    if (!progress.ret) {
        promise.reject.call({ QJSValue(QString::fromStdString(progress.ret.text())) });
        return promise.promise;
    }

    progress.val.finished().onReceive(this, [this, incoming, resolve = promise.resolve, reject = promise.reject]
                                      (const ProgressResult& res)
    {
        if (!res.ret) {
            reject.call({ QJSValue(QString::fromStdString(res.ret.text())) });
            return;
        }

        const int status = res.ret.data<int>("status", 0);
        const QString statusText = QString::fromStdString(res.ret.data<std::string>("statusText", std::string()));
        const QVariantMap headers = res.ret.data<QVariantMap>("headers", QVariantMap());
        resolve.call({ makeResponse(status, statusText, incoming->data(), headers) });
    });

    return promise.promise;
}

QJSValue FetchApi::toUtf8(const QJSValue& bytes) const
{
    if (bytes.isUndefined() || bytes.isNull()) {
        return QJSValue(QString());
    }

    QByteArray data = qjsvalue_cast<QByteArray>(bytes);
    return QJSValue(QString::fromUtf8(data));
}

QString FetchApi::stringOption(const QJSValue& options, const QString& key, const QString& def) const
{
    if (!options.isObject()) {
        return def;
    }

    const QJSValue value = options.property(key);
    if (!value.isString()) {
        return def;
    }

    return value.toString();
}

QByteArray FetchApi::byteArrayOption(const QJSValue& options, const QString& key, const QByteArray& def) const
{
    if (!options.isObject()) {
        return def;
    }

    const QJSValue value = options.property(key);
    if (value.isUndefined() || value.isNull()) {
        return def;
    }

    if (value.isString()) {
        return value.toString().toUtf8();
    }

    return qjsvalue_cast<QByteArray>(value);
}

RequestHeaders FetchApi::requestHeaders(const QJSValue& options) const
{
    RequestHeaders headers = configuration()->defaultHeaders();
    if (!options.isObject()) {
        return headers;
    }

    const QJSValue headersValue = options.property("headers");
    if (!headersValue.isObject()) {
        return headers;
    }

    QJSValueIterator it(headersValue);
    while (it.hasNext()) {
        it.next();
        if (!it.value().isString()) {
            continue;
        }

        const QString name = it.name();
        const QString value = it.value().toString();
        headers.rawHeaders.insert(name.toUtf8(), value.toUtf8());
    }

    return headers;
}

RetVal<Progress> FetchApi::execRequest(const QString& method, const QUrl& url,
                                       QIODevicePtr outgoing, QIODevicePtr incoming,
                                       const RequestHeaders& headers)
{
    if (!m_networkManager) {
        m_networkManager = networkManagerCreator()->makeNetworkManager();
    }

    if (method == QStringLiteral("GET")) {
        return m_networkManager->get(url, incoming, headers);
    }
    if (method == QStringLiteral("HEAD")) {
        return m_networkManager->head(url, headers);
    }
    if (method == QStringLiteral("DELETE")) {
        return m_networkManager->del(url, incoming, headers);
    }

    if (method == QStringLiteral("POST")) {
        return m_networkManager->post(url, outgoing, incoming, headers);
    }
    if (method == QStringLiteral("PUT")) {
        return m_networkManager->put(url, outgoing, incoming, headers);
    }
    if (method == QStringLiteral("PATCH")) {
        return m_networkManager->patch(url, outgoing, incoming, headers);
    }

    return RetVal<Progress>::make_ret(Ret::Code::NotSupported, "Unsupported method: " + method.toStdString());
}

QJSValue FetchApi::responseFactory()
{
    if (m_responseFactory.isCallable()) {
        return m_responseFactory;
    }

    m_responseFactory = engine()->evaluate(QStringLiteral(
                                               R"(
        (function (self, status, statusText, body, headers) {

            return {
                status: status,
                statusText: statusText,
                ok: status >= 200 && status < 300,
                headers: {
                    get: function (name) {
                        var key = String(name).toLowerCase()
                        return Object.prototype.hasOwnProperty.call(headers, key) ? headers[key] : null
                    },
                    has: function (name) {
                        return Object.prototype.hasOwnProperty.call(headers, String(name).toLowerCase())
                    }
                },
                text: function () {
                    return Promise.resolve(self.toUtf8(body))
                },
                json: function () {
                    return new Promise(function (resolve, reject) {
                        try {
                            resolve(JSON.parse(self.toUtf8(body)))
                        } catch (e) {
                            reject(e)
                        }
                    })
                },
                arrayBuffer: function () {
                    return Promise.resolve(body)
                }
            }
        })
    )"));

    if (m_responseFactory.isError()) {
        LOGE() << "failed to create response factory: " << m_responseFactory.toString();
    }

    return m_responseFactory;
}

QJSValue FetchApi::makeResponse(int status, const QString& statusText, const QByteArray& body, const QVariantMap& headers)
{
    QJSValue headersObj = engine()->newObject();
    for (auto it = headers.cbegin(); it != headers.cend(); ++it) {
        headersObj.setProperty(it.key(), it.value().toString());
    }

    return responseFactory().call({
        m_self,
        QJSValue(status),
        QJSValue(statusText),
        engine()->newArrayBuffer(body),
        headersObj
    });
}
