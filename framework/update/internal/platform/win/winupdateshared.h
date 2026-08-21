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

#include <string>
#include <vector>

#include <windows.h>

namespace muse::update::win {
inline std::wstring utf8ToWide(const std::string& str)
{
    if (str.empty()) {
        return std::wstring();
    }

    const int size = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
    if (size <= 0) {
        return std::wstring();
    }

    std::wstring result(static_cast<size_t>(size), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), result.data(), size);
    return result;
}

inline std::string wideToUtf8(const std::wstring& str)
{
    if (str.empty()) {
        return std::string();
    }

    const int size = ::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return std::string();
    }

    std::string result(static_cast<size_t>(size), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), result.data(), size, nullptr, nullptr);
    return result;
}

inline std::wstring programDataPath()
{
    wchar_t buffer[MAX_PATH] = { 0 };
    const DWORD size = ::GetEnvironmentVariableW(L"ProgramData", buffer, MAX_PATH);
    if (size > 0 && size < MAX_PATH) {
        return std::wstring(buffer, size);
    }

    return L"C:\\ProgramData";
}

//! `C:\ProgramData\Muse` and `C:\ProgramData\Muse\Update`.
//! Named because the levels above the working area have to be secured too,
//! not only the leaf.
inline std::wstring vendorRootPath()
{
    return programDataPath() + L"\\Muse";
}

inline std::wstring updatesRootPath()
{
    return vendorRootPath() + L"\\Update";
}

//! Root of the update working area, e.g. `C:\ProgramData\Muse\Update\MuseScore4`.
inline std::wstring updateRootPath(const std::wstring& appId)
{
    return updatesRootPath() + L"\\" + appId;
}

inline std::wstring requestsDirPath(const std::wstring& appId)
{
    return updateRootPath(appId) + L"\\requests";
}

inline std::wstring requestFilePath(const std::wstring& appId)
{
    return requestsDirPath(appId) + L"\\update.req";
}

inline std::wstring stagingDirPath(const std::wstring& appId)
{
    return updateRootPath(appId) + L"\\staging";
}

//! `packageType` doubles as the extension, and comes from the registry rather
//! than from the request: the helper executes this file, so an unprivileged
//! caller must have no say in what it is called.
inline std::wstring stagedPackagePath(const std::wstring& appId, const std::wstring& packageType)
{
    return stagingDirPath(appId) + L"\\update." + packageType;
}

//! The helper copies itself here before running the installer, so that the
//! installer replacing the install location never touches a running image.
inline std::wstring detachedHelperPath(const std::wstring& appId)
{
    return updateRootPath(appId) + L"\\museupdater-run.exe";
}

inline std::wstring logFilePath(const std::wstring& appId)
{
    return updateRootPath(appId) + L"\\museupdater.log";
}

inline std::wstring taskFolderName()
{
    return L"Muse";
}

inline std::wstring taskName(const std::wstring& appId)
{
    return appId + L" Update";
}

//! Full Task Scheduler path, e.g. `\Muse\MuseScore4 Update`.
inline std::wstring taskPath(const std::wstring& appId)
{
    return L"\\" + taskFolderName() + L"\\" + taskName(appId);
}

//! HKLM key written by the installer; the only trusted source of what to install
//! and what to relaunch. Nothing here is layout-specific: an application that
//! keeps its executable at the root of the install directory, or ships an Inno
//! Setup installer instead of an MSI, only registers different values.
inline std::wstring registryKeyPath(const std::wstring& appId)
{
    return L"SOFTWARE\\Muse\\Update\\" + appId;
}

//! Root of the installation.
inline constexpr const wchar_t* REG_VALUE_INSTALL_DIR = L"InstallDir";

//! Absolute path of the application executable, used both to relaunch it and to
//! tell whether a running application belongs to this installation.
inline constexpr const wchar_t* REG_VALUE_APP_PATH = L"AppPath";

//! "msi" (installed with msiexec) or "exe" (a self-contained installer, run
//! directly). Also the extension the staged package is given.
inline constexpr const wchar_t* REG_VALUE_PACKAGE_TYPE = L"PackageType";

//! Extra arguments for the silent installation, e.g. `INSTALL_ROOT={install-dir}`
//! for an MSI or `/VERYSILENT /NORESTART /DIR={install-dir}` for Inno Setup. The
//! `{install-dir}` token expands to the install location, already quoted.
inline constexpr const wchar_t* REG_VALUE_INSTALL_ARGS = L"InstallArgs";

//! Expected common name of the signing certificate; several may be given,
//! separated by "|".
//!
//! This is written by the installer of the version that is *currently* running,
//! so a package is always judged against what the previous release knew about.
//! Accepting more than one name is what makes it possible to rotate the
//! certificate: ship the new name alongside the old one first, and only start
//! signing with it once that release is out.
inline constexpr const wchar_t* REG_VALUE_CERT_SUBJECT = L"CertSubject";

//! Whether `signer` is among the "|"-separated names in `expected`. Empty
//! `expected` matches nothing - there is then nothing to verify against.
inline bool isExpectedSigner(const std::wstring& signer, const std::wstring& expected)
{
    if (signer.empty() || expected.empty()) {
        return false;
    }

    size_t pos = 0;
    while (pos <= expected.size()) {
        size_t end = expected.find(L'|', pos);
        if (end == std::wstring::npos) {
            end = expected.size();
        }

        std::wstring name = expected.substr(pos, end - pos);

        // Tolerate spaces around the separator.
        const size_t first = name.find_first_not_of(L" \t");
        const size_t last = name.find_last_not_of(L" \t");
        if (first != std::wstring::npos) {
            name = name.substr(first, last - first + 1);
        } else {
            name.clear();
        }

        if (!name.empty() && name == signer) {
            return true;
        }

        pos = end + 1;
    }

    return false;
}

inline constexpr const wchar_t* PACKAGE_TYPE_MSI = L"msi";
inline constexpr const wchar_t* PACKAGE_TYPE_EXE = L"exe";

//! Replaces `{install-dir}` in `args` with `installDir` in quotes.
inline std::wstring expandInstallArgs(const std::wstring& args, const std::wstring& installDir)
{
    const std::wstring token = L"{install-dir}";
    const std::wstring value = L"\"" + installDir + L"\"";

    std::wstring result = args;
    for (size_t pos = result.find(token); pos != std::wstring::npos; pos = result.find(token, pos + value.size())) {
        result.replace(pos, token.size(), value);
    }

    return result;
}

struct UpdateUi {
    std::wstring title;
    std::wstring message;
    std::wstring backgroundColor;
    std::wstring accentColor;
    std::wstring foregroundColor;

    bool isValid() const { return !message.empty(); }
};

//! Control characters would break the line-oriented formats this travels
//! through; the length is bounded because the window shows a single line.
inline std::wstring sanitizedUiText(const std::wstring& text)
{
    const size_t maxLength = 200;

    std::wstring result;
    result.reserve(text.size() < maxLength ? text.size() : maxLength);

    for (const wchar_t c : text) {
        if (result.size() >= maxLength) {
            break;
        }

        if (c == L'\t' || (c >= L' ' && c != 0x7F)) {
            result.push_back(c);
        }
    }

    return result;
}

//! `#rrggbb`
inline bool parseUiColor(const std::wstring& color, COLORREF& parsed)
{
    if (color.size() != 7 || color[0] != L'#') {
        return false;
    }

    int component[3] = { 0, 0, 0 };

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 2; ++j) {
            const wchar_t c = color[1 + i * 2 + j];
            int digit = 0;
            if (c >= L'0' && c <= L'9') {
                digit = c - L'0';
            } else if (c >= L'a' && c <= L'f') {
                digit = c - L'a' + 10;
            } else if (c >= L'A' && c <= L'F') {
                digit = c - L'A' + 10;
            } else {
                return false;
            }

            component[i] = component[i] * 16 + digit;
        }
    }

    parsed = RGB(component[0], component[1], component[2]);
    return true;
}

inline bool isUiColor(const std::wstring& color)
{
    COLORREF ignored = 0;
    return parseUiColor(color, ignored);
}

struct UpdateRequest {
    std::wstring packagePath;
    unsigned long long pid = 0;
    UpdateUi ui;
};

inline bool writeFileContent(const std::wstring& path, const std::string& content)
{
    const HANDLE file = ::CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    const BOOL ok = ::WriteFile(file, content.data(), static_cast<DWORD>(content.size()), &written, nullptr);
    ::CloseHandle(file);

    return ok && written == content.size();
}

inline bool readFileContent(const std::wstring& path, std::string& content)
{
    const HANDLE file = ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    LARGE_INTEGER size = { };
    if (!::GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 64 * 1024) {
        ::CloseHandle(file);
        return false;
    }

    content.resize(static_cast<size_t>(size.QuadPart));

    DWORD read = 0;
    const BOOL ok = ::ReadFile(file, content.data(), static_cast<DWORD>(content.size()), &read, nullptr);
    ::CloseHandle(file);

    if (!ok) {
        return false;
    }

    content.resize(read);
    return true;
}

inline bool writeRequest(const std::wstring& appId, const UpdateRequest& request)
{
    std::string content;
    content += "package=" + wideToUtf8(request.packagePath) + "\n";
    content += "pid=" + std::to_string(request.pid) + "\n";

    auto appendUiValue = [&content](const char* key, const std::wstring& value) {
        if (!value.empty()) {
            content += std::string(key) + "=" + wideToUtf8(value) + "\n";
        }
    };

    appendUiValue("ui.title", sanitizedUiText(request.ui.title));
    appendUiValue("ui.message", sanitizedUiText(request.ui.message));

    if (isUiColor(request.ui.backgroundColor)) {
        appendUiValue("ui.background", request.ui.backgroundColor);
    }
    if (isUiColor(request.ui.accentColor)) {
        appendUiValue("ui.accent", request.ui.accentColor);
    }
    if (isUiColor(request.ui.foregroundColor)) {
        appendUiValue("ui.foreground", request.ui.foregroundColor);
    }

    return writeFileContent(requestFilePath(appId), content);
}

inline bool readRequest(const std::wstring& appId, UpdateRequest& request)
{
    std::string content;
    if (!readFileContent(requestFilePath(appId), content)) {
        return false;
    }

    //! NOTE: The application writes no byte order mark, but a request written by
    //! hand while testing usually has one.
    if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF
        && static_cast<unsigned char>(content[1]) == 0xBB && static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }

    size_t pos = 0;
    while (pos < content.size()) {
        size_t end = content.find('\n', pos);
        if (end == std::string::npos) {
            end = content.size();
        }

        std::string line = content.substr(pos, end - pos);
        pos = end + 1;

        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }

        const size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }

        const std::string key = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);

        if (key == "package") {
            request.packagePath = utf8ToWide(value);
        } else if (key == "pid") {
            try {
                request.pid = std::stoull(value);
            } catch (...) {
                request.pid = 0;
            }
        } else if (key == "ui.title") {
            request.ui.title = sanitizedUiText(utf8ToWide(value));
        } else if (key == "ui.message") {
            request.ui.message = sanitizedUiText(utf8ToWide(value));
        } else if (key == "ui.background") {
            const std::wstring color = utf8ToWide(value);
            request.ui.backgroundColor = isUiColor(color) ? color : std::wstring();
        } else if (key == "ui.accent") {
            const std::wstring color = utf8ToWide(value);
            request.ui.accentColor = isUiColor(color) ? color : std::wstring();
        } else if (key == "ui.foreground") {
            const std::wstring color = utf8ToWide(value);
            request.ui.foregroundColor = isUiColor(color) ? color : std::wstring();
        }
    }

    return !request.packagePath.empty();
}
}
