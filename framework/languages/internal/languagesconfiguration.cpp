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
#include "languagesconfiguration.h"

#include <QStringList>
#include <QUrl>

#include "global/configreader.h"
#include "settings.h"
#include "languagestypes.h"

#include "log.h"

using namespace muse;
using namespace muse::languages;

static const Settings::Key LANGUAGE_KEY("languages", "language");
static const Settings::Key NUMBER_FORMAT_SOURCE_KEY("languages", "numberFormatSource");

void LanguagesConfiguration::init()
{
    m_config = ConfigReader::read(":/configs/languages.cfg");

    settings()->setDefaultValue(LANGUAGE_KEY, Val(SYSTEM_LANGUAGE_CODE.toStdString()));
    settings()->valueChanged(LANGUAGE_KEY).onReceive(nullptr, [this](const Val& val) {
        m_currentLanguageCodeChanged.send(val.toQString());
    });

    settings()->setDefaultValue(NUMBER_FORMAT_SOURCE_KEY, Val(SYSTEM_NUMBER_FORMAT_SOURCE.toStdString()));
    settings()->valueChanged(NUMBER_FORMAT_SOURCE_KEY).onReceive(nullptr, [this](const Val& val) {
        m_numberFormatSourceChanged.send(val.toQString());
    });
}

ValCh<QString> LanguagesConfiguration::currentLanguageCode() const
{
    ValCh<QString> result;
    result.ch = m_currentLanguageCodeChanged;
    result.val = settings()->value(LANGUAGE_KEY).toQString();

    return result;
}

void LanguagesConfiguration::setCurrentLanguageCode(const QString& languageCode) const
{
    Val value(languageCode.toStdString());
    settings()->setSharedValue(LANGUAGE_KEY, value);
}

ValCh<QString> LanguagesConfiguration::numberFormatSource() const
{
    ValCh<QString> result;
    result.ch = m_numberFormatSourceChanged;
    result.val = settings()->value(NUMBER_FORMAT_SOURCE_KEY).toQString();

    return result;
}

void LanguagesConfiguration::setNumberFormatSource(const QString& source) const
{
    settings()->setSharedValue(NUMBER_FORMAT_SOURCE_KEY, Val(source.toStdString()));
}

QUrl LanguagesConfiguration::languagesUpdateUrl() const
{
    return QUrl(m_config.value("server_url").toQString() + "details.json");
}

QUrl LanguagesConfiguration::languageFileServerUrl(const QString& languageCode) const
{
    TRACEFUNC;
    return QUrl(m_config.value("server_url").toQString() + QString("locale_%1.zip").arg(languageCode));
}

io::path_t LanguagesConfiguration::languagesAppDataPath() const
{
    return globalConfiguration()->appDataPath() + "/locale";
}

io::path_t LanguagesConfiguration::languagesUserAppDataPath() const
{
    return globalConfiguration()->userAppDataPath() + "/locale";
}

io::path_t LanguagesConfiguration::builtinLanguagesJsonPath() const
{
    return ":/languages/resources/languages.json";
}

io::path_t LanguagesConfiguration::builtinLanguageFilePath(const QString& resourceName, const QString& languageCode) const
{
    return languagesAppDataPath() + QString("/%1_%2.qm").arg(resourceName, languageCode);
}

io::path_t LanguagesConfiguration::userLanguageFilePath(const QString& resourceName, const QString& languageCode) const
{
    return languagesUserAppDataPath() + QString("/%1_%2.qm").arg(resourceName, languageCode);
}

QStringList LanguagesConfiguration::languageResourceNames() const
{
    const Val& val = m_config.value("resource_names");
    QStringList result;
    for (const Val& item : val.toList()) {
        result.append(item.toQString());
    }
    return result;
}
