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

#include "interactiveapi.h"

#include <QUrl>
#include <QMetaEnum>

using namespace muse::interactive;

/** APIDOC
 * User interaction - informational messages, error messages, questions and other dialogs.
 * @namespace Interactive
 * @example
 * const Interactive = require("MuseApi.Interactive");
 *
 * function main() {
 *     Interactive.info("Quick start", "called main function")
 * }
 */

InteractiveApi::InteractiveApi(api::IApiEngine* e)
    : ApiObject(e)
{
}

/** APIDOC
 * Show information message
 * @method
 * @param {String} title Title
 * @param {String} text Message
 * @return {Promise} - resolved when the message is closed
 * @example
 * Interactive.info("My title", "Some information").then(function() {
 *     ...
 * })
 */
QJSValue InteractiveApi::info(const QString& contentTitle, const QString& text)
{
    api::JsPromise promise = engine()->newPromise();

    interactive()->info(contentTitle.toStdString(), IInteractive::Text(text.toStdString()))
    .onResolve(this, [resolve = promise.resolve](const IInteractive::Result&) {
        resolve.call();
    });

    return promise.promise;
}

/** APIDOC
 * Show warning message
 * @method
 * @param {String} title Title
 * @param {String} text Message
 * @return {Promise} - resolved when the message is closed
 * @example
 * Interactive.warning("My title", "Some warning").then(function() {
 *     ...
 * })
 */
QJSValue InteractiveApi::warning(const QString& contentTitle, const QString& text)
{
    api::JsPromise promise = engine()->newPromise();

    interactive()->warning(contentTitle.toStdString(), IInteractive::Text(text.toStdString()))
    .onResolve(this, [resolve = promise.resolve](const IInteractive::Result&) {
        resolve.call();
    });

    return promise.promise;
}

/** APIDOC
 * Show error message
 * @method
 * @param {String} title Title
 * @param {String} text Message
 * @return {Promise} - resolved when the message is closed
 * @example
 * Interactive.error("My title", "Some error").then(function() {
 *     ...
 * })
 */
QJSValue InteractiveApi::error(const QString& contentTitle, const QString& text)
{
    api::JsPromise promise = engine()->newPromise();

    interactive()->error(contentTitle.toStdString(), IInteractive::Text(text.toStdString()))
    .onResolve(this, [resolve = promise.resolve](const IInteractive::Result&) {
        resolve.call();
    });

    return promise.promise;
}

static muse::IInteractive::Button buttonFromString(const QString& str)
{
    static QMetaEnum meta = QMetaEnum::fromType<InteractiveApi::ButtonCode>();
    int val = meta.keyToValue(str.toLatin1().constData());
    if (val == -1) {
        return muse::IInteractive::Button::NoButton;
    }
    return static_cast<muse::IInteractive::Button>(val);
}

static QString buttonToString(const muse::IInteractive::Button& btn)
{
    static QMetaEnum meta = QMetaEnum::fromType<InteractiveApi::ButtonCode>();
    const char* key = meta.valueToKey(static_cast<int>(btn));
    return QString::fromLatin1(key);
}

std::vector<muse::IInteractive::Button> InteractiveApi::buttons(const QJSValueList& btns) const
{
    std::vector<muse::IInteractive::Button> result;
    for (const QJSValue& btn : btns) {
        QString str = btn.toString();
        result.push_back(buttonFromString(str));
    }
    return result;
}

/** APIDOC
 * Ask a question
 * @method
 * @param {String} title Title
 * @param {String} text Message
 * @param {ButtonCode[]} buttons Code of buttons
 * @return {Promise<ButtonCode>} - selected button
 * @example
 * api.interactive.question("My question", "Yes or No?", [ButtonCode.Yes, ButtonCode.No]).then(function(btn) {
 *     if (btn === ButtonCode.Yes) {
 *         ...
 *     }
 * })
 */
QJSValue InteractiveApi::question(const QString& contentTitle, const QString& text, const QJSValueList& btns)
{
    api::JsPromise promise = engine()->newPromise();

    interactive()->question(contentTitle.toStdString(), text.toStdString(), buttons(btns))
    .onResolve(this, [resolve = promise.resolve](const IInteractive::Result& res) {
        resolve.call({ QJSValue(buttonToString(res.standardButton())) });
    });

    return promise.promise;
}

/** APIDOC
 * Open URL in external browser
 * @method
 * @param {String} url URL
 */
void InteractiveApi::openUrl(const QString& url)
{
    platformInteractive()->openUrl(QUrl(url));
}
