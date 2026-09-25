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

#include "filesystemapirestriction.h"

#include <sstream>

#include "global/io/fileinfo.h"

#include "log.h"

using namespace muse;
using namespace muse::api;

namespace {
constexpr int MAX_SYMLINK_DEPTH = 40;

std::string parentPath(const std::string& path)
{
    if (path.empty() || path == "/") {
        return "/";
    }

#ifdef Q_OS_WIN
    if (path.size() <= 3 && path.size() >= 2 && path[1] == ':') {
        return path.size() == 2 ? path + "/" : path;
    }
    if (path.size() >= 2 && path[0] == '/' && path[1] == '/') {
        const size_t serverEnd = path.find('/', 2);
        if (serverEnd == std::string::npos) {
            return path;
        }
        const size_t shareEnd = path.find('/', serverEnd + 1);
        if (shareEnd == std::string::npos) {
            return path;
        }
    }
#endif

    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) {
        return path;
    }
    if (slash == 0) {
        return "/";
    }
#ifdef Q_OS_WIN
    if (slash == 2 && path[1] == ':') {
        return path.substr(0, 3);
    }
#endif
    return path.substr(0, slash);
}

std::string joinPath(const std::string& base, const std::string& component)
{
    if (base.empty() || base == "/") {
        return "/" + component;
    }
    if (base.back() == '/') {
        return base + component;
    }
    return base + "/" + component;
}

void stripTrailingSlash(std::string& path)
{
    while (path.size() > 1 && path.back() == '/') {
#ifdef Q_OS_WIN
        if (path.size() == 3 && path[1] == ':') {
            break;
        }
#endif
        path.pop_back();
    }
}

bool isUnder(const std::string& target, std::string base)
{
    stripTrailingSlash(base);
    if (target == base) {
        return true;
    }
    return target.size() > base.size()
           && target.compare(0, base.size(), base) == 0
           && target[base.size()] == '/';
}

std::string resolveWithDepth(const io::path_t& path, int depth)
{
    if (depth > MAX_SYMLINK_DEPTH || path.empty()) {
        return {};
    }

    const std::string absolute = io::FileInfo(path).absoluteFilePath().toStdString();
    if (absolute.empty()) {
        return {};
    }

    std::string normalized;
    normalized.reserve(absolute.size());
    for (const char c : absolute) {
        normalized.push_back(c == '\\' ? '/' : c);
    }

    std::string current;
    size_t pos = 0;
#ifdef Q_OS_WIN
    if (normalized.size() >= 2 && normalized[1] == ':') {
        current = normalized.substr(0, 2) + "/";
        pos = (normalized.size() > 2 && normalized[2] == '/') ? 3 : 2;
    } else if (normalized.size() >= 2 && normalized[0] == '/' && normalized[1] == '/') {
        const size_t serverEnd = normalized.find('/', 2);
        if (serverEnd == std::string::npos) {
            return {};
        }
        const size_t shareEnd = normalized.find('/', serverEnd + 1);
        if (shareEnd == std::string::npos) {
            current = normalized;
            pos = normalized.size();
        } else {
            current = normalized.substr(0, shareEnd);
            pos = shareEnd + 1;
        }
    } else
#endif
    if (!normalized.empty() && normalized[0] == '/') {
        current = "/";
        pos = 1;
    } else {
        return {};
    }

    while (pos < normalized.size()) {
        const size_t nextSlash = normalized.find('/', pos);
        const std::string component = normalized.substr(
            pos, nextSlash == std::string::npos ? std::string::npos : nextSlash - pos);
        pos = nextSlash == std::string::npos ? normalized.size() : nextSlash + 1;

        if (component.empty() || component == ".") {
            continue;
        }
        if (component == "..") {
            current = parentPath(current);
            continue;
        }

        const std::string next = joinPath(current, component);
        const io::FileInfo info(next);
        std::string canonical = info.canonicalFilePath().toStdString();
        if (!canonical.empty()) {
            for (char& c : canonical) {
                if (c == '\\') {
                    c = '/';
                }
            }
            current = canonical;
            continue;
        }

        if (info.isSymLink()) {
            const io::path_t target = info.symLinkTarget();
            if (target.empty()) {
                return {};
            }
            const std::string resolvedTarget = resolveWithDepth(target, depth + 1);
            if (resolvedTarget.empty()) {
                return {};
            }
            current = resolvedTarget;
            continue;
        }

        current = next;
    }

    stripTrailingSlash(current);
    return current;
}
}

std::string FileSystemApiRestriction::resolvedPath(const io::path_t& path)
{
    return resolveWithDepth(path, 0);
}

void FileSystemApiRestriction::addAllowedPathBase(const std::string& key, const io::path_t& path)
{
    if (path.empty()) {
        m_allowedPathBases.erase(key);
        return;
    }

    const std::string resolved = resolvedPath(path);
    if (resolved.empty()) {
        m_allowedPathBases.erase(key);
        return;
    }

    m_allowedPathBases[key] = resolved;
}

void FileSystemApiRestriction::removeAllowedPathBase(const std::string& key)
{
    m_allowedPathBases.erase(key);
}

void FileSystemApiRestriction::clearAllowedPathBases()
{
    m_allowedPathBases.clear();
}

muse::Ret FileSystemApiRestriction::isPathAllowed(const io::path_t& path) const
{
    if (m_allowedPathBases.empty()) {
        return muse::make_ret(Ret::Code::NotAllowed, "Path is not allowed");
    }

    const std::string resolved = resolvedPath(path);
    if (resolved.empty()) {
        LOGW() << "File blocked: path '" << path << "' could not be resolved";
        return muse::make_ret(Ret::Code::NotAllowed, "Path is not allowed");
    }

    bool allowed = false;
    for (const auto& p : m_allowedPathBases) {
        if (isUnder(resolved, p.second)) {
            allowed = true;
            break;
        }
    }

    if (!allowed) {
        LOGW() << "File blocked: path '" << resolved << "' is outside allowed directories";
        std::stringstream ss;
        ss << "Allowed directories: ";
        for (const auto& p : m_allowedPathBases) {
            ss << p.first << ": " << p.second << "\n";
        }
        LOGW() << ss.str();
    }

    return allowed ? muse::make_ok() : muse::make_ret(Ret::Code::NotAllowed, "Path is not allowed");
}
