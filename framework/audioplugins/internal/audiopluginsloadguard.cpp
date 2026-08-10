/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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
#include "audiopluginsloadguard.h"

#include <functional>
#include <sstream>

#include "log.h"

using namespace muse;
using namespace muse::audioplugins;

static const std::string SENTINEL_SUFFIX = ".loading";

Ret AudioPluginsLoadGuard::beginLoad(const PluginResourceId& resourceId)
{
    IF_ASSERT_FAILED(!resourceId.empty()) {
        return make_ret(Ret::Code::UnknownError);
    }

    Ret ret = fileSystem()->makePath(sentinelsDirPath());
    if (!ret) {
        LOGE() << "Failed to create plugin load guard dir: " << ret.toString();
        return ret;
    }

    ByteArray data(resourceId.data(), resourceId.size());
    ret = fileSystem()->writeFile(sentinelFilePath(resourceId), data);
    if (!ret) {
        LOGE() << "Failed to write plugin load sentinel for " << resourceId << ": " << ret.toString();
    }

    return ret;
}

void AudioPluginsLoadGuard::endLoad(const PluginResourceId& resourceId)
{
    IF_ASSERT_FAILED(!resourceId.empty()) {
        return;
    }

    Ret ret = fileSystem()->remove(sentinelFilePath(resourceId));
    if (!ret) {
        LOGE() << "Failed to remove plugin load sentinel for " << resourceId << ": " << ret.toString();
    }
}

PluginResourceIdList AudioPluginsLoadGuard::danglingLoads() const
{
    const io::path_t dir = sentinelsDirPath();
    if (!fileSystem()->exists(dir)) {
        return {};
    }

    const RetVal<io::paths_t> files = fileSystem()->scanFiles(dir, { "*" + SENTINEL_SUFFIX }, io::ScanMode::FilesInCurrentDir);
    if (!files.ret) {
        LOGE() << "Failed to scan plugin load guard dir: " << files.ret.toString();
        return {};
    }

    PluginResourceIdList result;
    result.reserve(files.val.size());

    for (const io::path_t& file : files.val) {
        RetVal<ByteArray> data = fileSystem()->readFile(file);
        if (!data.ret) {
            LOGE() << "Failed to read plugin load sentinel " << file.toStdString() << ": " << data.ret.toString();
            continue;
        }

        // the file name is a hash; the content holds the actual id
        std::string resourceId(reinterpret_cast<const char*>(data.val.constData()), data.val.size());
        if (!resourceId.empty()) {
            result.push_back(std::move(resourceId));
        }
    }

    return result;
}

Ret AudioPluginsLoadGuard::clearDanglingLoads()
{
    const io::path_t dir = sentinelsDirPath();
    if (!fileSystem()->exists(dir)) {
        return make_ok();
    }

    return fileSystem()->clear(dir);
}

io::path_t AudioPluginsLoadGuard::sentinelsDirPath() const
{
    return globalConfiguration()->userAppDataPath() + "/audio_plugins_loading";
}

io::path_t AudioPluginsLoadGuard::sentinelFilePath(const PluginResourceId& resourceId) const
{
    // ids may contain characters that are not valid in file names; use a hash
    // for the name and keep the authoritative id in the file content
    std::ostringstream name;
    name << std::hex << std::hash<std::string> {}(resourceId);
    return sentinelsDirPath() + "/" + name.str() + SENTINEL_SUFFIX;
}
