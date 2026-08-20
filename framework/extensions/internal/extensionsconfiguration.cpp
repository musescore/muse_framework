/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited and others
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
#include "extensionsconfiguration.h"

#include "global/settings.h"
#include "global/serialization/json.h"
#include "global/io/file.h"
#include "global/io/dir.h"
#include "multiwindows/resourcelockguard.h"
#include "legacy/extpluginsloader.h"

#include "log.h"

using namespace muse;
using namespace muse::extensions;

static const muse::Settings::Key USER_PLUGINS_PATH("plugins", "application/paths/myPlugins");

static const std::string EXTENSIONS_RESOURCE_NAME("EXTENSIONS");

void ExtensionsConfiguration::init()
{
    settings()->setDefaultValue(USER_PLUGINS_PATH, Val(globalConfiguration()->userDataPath() + "/Plugins"));
    settings()->valueChanged(USER_PLUGINS_PATH).onReceive(nullptr, [this](const Val& val) {
        m_pluginsUserPathChanged.send(val.toString());
    });

    if (!pluginsUserPath().empty()) {
        io::Dir::mkpath(pluginsUserPath());
    }

    io::Dir::mkpath(userPath());
}

io::path_t ExtensionsConfiguration::defaultPath() const
{
    return globalConfiguration()->appDataPath() + "/extensions";
}

io::path_t ExtensionsConfiguration::userPath() const
{
    return globalConfiguration()->userAppDataPath() + "/extensions";
}

io::path_t ExtensionsConfiguration::pluginsDefaultPath() const
{
    return globalConfiguration()->appDataPath() + "/plugins";
}

io::path_t ExtensionsConfiguration::pluginsUserPath() const
{
    return settings()->value(USER_PLUGINS_PATH).toPath();
}

void ExtensionsConfiguration::setUserPluginsPath(const io::path_t& path)
{
    settings()->setSharedValue(USER_PLUGINS_PATH, Val(path));
}

async::Channel<io::path_t> ExtensionsConfiguration::pluginsUserPathChanged() const
{
    return m_pluginsUserPathChanged;
}

Ret ExtensionsConfiguration::setExtensionConfigs(const std::map<Uri, ExtensionConfig>& configs)
{
    JsonArray arr;
    for (const auto& p : configs) {
        JsonObject obj;
        obj["uri"] = p.first.toString();

        const ExtensionConfig& c = p.second;
        obj["enabled"] = c.enabled;

        arr.append(obj);
    }

    Ret ret;
    ByteArray data = JsonDocument(arr).toJson();
    {
        mi::WriteResourceLockGuard lock_guard(multiwindowsProvider.get(), EXTENSIONS_RESOURCE_NAME);
        ret = io::File::writeFile(userPath() + "/config.json", data);
    }

    if (!ret) {
        LOGE() << "failed write config data, err: " << ret.toString();
    }

    return ret;
}

std::map<muse::Uri, ExtensionConfig> ExtensionsConfiguration::extensionConfigs() const
{
    const io::path_t configPath = userPath() + "/config.json";
    const io::path_t oldPluginsConfigPath = globalConfiguration()->userAppDataPath() + "/plugins/plugins.json";

    //! NOTE Load current config
    if (io::File::exists(configPath)) {
        ByteArray data;
        {
            mi::ReadResourceLockGuard lock_guard(multiwindowsProvider.get(), EXTENSIONS_RESOURCE_NAME);
            Ret ret = io::File::readFile(configPath, data);
            if (!ret) {
                LOGE() << "failed read config data, err: " << ret.toString() << ", file: " << configPath;
                return {};
            }
        }

        std::string err;
        JsonDocument doc = JsonDocument::fromJson(data, &err);
        if (!err.empty()) {
            LOGE() << "failed parse json, err: " << err;
            return {};
        }

        if (!doc.isArray()) {
            LOGE() << "bad format of config file";
            return {};
        }

        std::map<Uri, ExtensionConfig> result;
        JsonArray arr = doc.rootArray();
        for (size_t i = 0; i < arr.size(); ++i) {
            JsonObject obj = arr.at(i).toObject();

            Uri uri = Uri(obj.value("uri").toStdString());

            ExtensionConfig c;
            c.enabled = obj.value("enabled").toBool();

            result.insert({ uri, c });
        }

        return result;
    }
    //! NOTE Load old plugins config
    else {
        ByteArray data;

        {
            mi::ReadResourceLockGuard lock_guard(multiwindowsProvider.get(), EXTENSIONS_RESOURCE_NAME);
            Ret ret = io::File::readFile(oldPluginsConfigPath, data);
            if (!ret) {
                LOGE() << "failed read config data, err: " << ret.toString() << ", file: " << oldPluginsConfigPath;
                return {};
            }
        }

        std::string err;
        JsonDocument doc = JsonDocument::fromJson(data, &err);
        if (!err.empty()) {
            LOGE() << "failed parse json, err: " << err;
            return {};
        }

        if (!doc.isArray()) {
            LOGE() << "bad format of config file";
            return {};
        }

        std::map<std::string /*codeKey*/, Uri> uris;
        {
            legacy::ExtPluginsLoader pluginsLoader;
            uris = pluginsLoader.loadCodekeyUriMap(pluginsDefaultPath(), pluginsUserPath());
        }

        std::map<Uri, ExtensionConfig> result;
        JsonArray arr = doc.rootArray();
        for (size_t i = 0; i < arr.size(); ++i) {
            JsonObject obj = arr.at(i).toObject();

            std::string codeKey = obj.value("codeKey").toStdString();
            Uri uri = muse::value(uris, codeKey);
            if (!uri.isValid()) {
                LOGW() << "not found plugin with codeKey: " << codeKey;
                continue;
            }

            ExtensionConfig c;
            c.enabled = obj.value("enabled").toBool();

            result.insert({ uri, c });
        }

        return result;
    }
}
