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
 */

#include "updatetask_win.h"

#include <windows.h>
#include <shellapi.h>
#include <taskschd.h>
#include <aclapi.h>
#include <sddl.h>
#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>
#include <userenv.h>
#include <wtsapi32.h>
#include <msi.h>
#include <msiquery.h>

#include <map>
#include <string>
#include <vector>

#include "platform.h"
#include "updateui_win.h"

#include "../internal/platform/win/winupdateshared.h"

//! The layout shared with the application: paths, the registry key and the
//! format of the request file.
namespace shared = muse::update::win;

namespace {
const wchar_t* TASK_AUTHOR = L"Muse";

//! Users (BU) get read + execute so that an unprivileged application can start
//! the task on demand; only administrators and SYSTEM may change it.
const wchar_t* TASK_SDDL = L"D:P(A;;GA;;;BA)(A;;GA;;;SY)(A;;GRGX;;;BU)";

//! Users may traverse and read the working area but write nothing into it: the
//! helper copies itself here before running the installer, and an unprivileged
//! user must not be able to plant anything we then execute as SYSTEM.
const wchar_t* ROOT_SDDL = L"O:BAD:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;GRGX;;;BU)";

//! SYSTEM and administrators only - the package is verified and installed from
//! here, so an unprivileged user must not be able to touch it.
const wchar_t* STAGING_SDDL = L"O:BAD:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)";

//! Users (BU) may read, write and delete requests. Whoever writes one does not
//! own it exclusively, so a request left behind by one user cannot keep another
//! from placing theirs. The contents are untrusted either way.
const wchar_t* REQUESTS_SDDL = L"O:BAD:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;GRGWGXSD;;;BU)";

HANDLE g_logFile = INVALID_HANDLE_VALUE;

void logLine(const std::wstring& message)
{
    if (g_logFile == INVALID_HANDLE_VALUE) {
        return;
    }

    SYSTEMTIME time = { };
    ::GetLocalTime(&time);

    auto padded = [](int value, size_t width) {
        std::wstring str = std::to_wstring(value);
        while (str.size() < width) {
            str.insert(str.begin(), L'0');
        }
        return str;
    };

    const std::wstring stamp = padded(time.wYear, 4) + L"-" + padded(time.wMonth, 2) + L"-" + padded(time.wDay, 2)
                               + L" " + padded(time.wHour, 2) + L":" + padded(time.wMinute, 2) + L":" + padded(time.wSecond, 2)
                               + L" ";

    const std::string line = shared::wideToUtf8(stamp + message + L"\r\n");

    DWORD written = 0;
    ::WriteFile(g_logFile, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
    ::FlushFileBuffers(g_logFile);
}

void openLog(const std::wstring& appId)
{
    const std::wstring path = shared::logFilePath(appId);
    g_logFile = ::CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
}

void closeLog()
{
    if (g_logFile != INVALID_HANDLE_VALUE) {
        ::CloseHandle(g_logFile);
        g_logFile = INVALID_HANDLE_VALUE;
    }
}

std::wstring quoted(const std::wstring& str)
{
    return L"\"" + str + L"\"";
}

std::wstring trimTrailingSeparators(const std::wstring& path)
{
    std::wstring result = path;
    while (result.size() > 3 && (result.back() == L'\\' || result.back() == L'/')) {
        result.pop_back();
    }
    return result;
}

std::wstring parentDir(const std::wstring& path)
{
    const std::wstring trimmed = trimTrailingSeparators(path);
    const size_t pos = trimmed.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return std::wstring();
    }
    return trimmed.substr(0, pos);
}

std::wstring modulePath()
{
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD size = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (size == 0) {
            return std::wstring();
        }

        if (size < buffer.size() - 1) {
            return std::wstring(buffer.data(), size);
        }

        buffer.resize(buffer.size() * 2);
    }
}

bool makeDirectories(const std::wstring& path)
{
    if (path.empty()) {
        return false;
    }

    size_t pos = path.find_first_of(L"\\/", path.find(L":\\") != std::wstring::npos ? 3 : 0);
    while (pos != std::wstring::npos) {
        const std::wstring part = path.substr(0, pos);
        if (!part.empty()) {
            ::CreateDirectoryW(part.c_str(), nullptr);
        }
        pos = path.find_first_of(L"\\/", pos + 1);
    }

    ::CreateDirectoryW(path.c_str(), nullptr);

    const DWORD attributes = ::GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

//! Takes ownership of `path` and replaces its inherited DACL with `sddl`.
//!
//! Ownership matters as much as the DACL here: any user can create
//! `%ProgramData%\Muse\...` ahead of the installer, and the owner of a directory
//! can always rewrite its DACL - resetting the permissions of a directory
//! somebody else owns would achieve nothing.
bool secureDirectory(const std::wstring& path, const wchar_t* sddl)
{
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl, SDDL_REVISION_1, &descriptor, nullptr)) {
        return false;
    }

    BOOL daclPresent = FALSE;
    BOOL daclDefaulted = FALSE;
    PACL dacl = nullptr;

    PSID owner = nullptr;
    BOOL ownerDefaulted = FALSE;

    bool ok = false;

    if (::GetSecurityDescriptorDacl(descriptor, &daclPresent, &dacl, &daclDefaulted) && daclPresent
        && ::GetSecurityDescriptorOwner(descriptor, &owner, &ownerDefaulted) && owner) {
        const DWORD result = ::SetNamedSecurityInfoW(const_cast<wchar_t*>(path.c_str()), SE_FILE_OBJECT,
                                                     OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION
                                                     | PROTECTED_DACL_SECURITY_INFORMATION,
                                                     owner, nullptr, dacl, nullptr);
        ok = result == ERROR_SUCCESS;
    }

    ::LocalFree(descriptor);
    return ok;
}

//! Creates the working area and makes sure it belongs to us, whether or not it
//! already existed. All callers run as SYSTEM or elevated from the installer.
bool ensureSecureRoot(const std::wstring& appId)
{
    const std::wstring root = shared::updateRootPath(appId);
    return makeDirectories(root) && secureDirectory(root, ROOT_SDDL);
}

//! Copying can transiently fail while an antivirus or the shell holds the file.
bool copyFileWithRetries(const std::wstring& from, const std::wstring& to, int attempts = 30)
{
    for (int i = 0; i < attempts; ++i) {
        if (::CopyFileW(from.c_str(), to.c_str(), FALSE)) {
            return true;
        }
        ::Sleep(200);
    }

    return false;
}

bool regWriteString(const std::wstring& subKey, const wchar_t* name, const std::wstring& value)
{
    HKEY key = nullptr;
    LSTATUS status = ::RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                                       KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &key, nullptr);
    if (status != ERROR_SUCCESS) {
        return false;
    }

    const DWORD size = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    status = ::RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), size);
    ::RegCloseKey(key);

    return status == ERROR_SUCCESS;
}

std::wstring regReadString(const std::wstring& subKey, const wchar_t* name)
{
    HKEY key = nullptr;
    if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        return std::wstring();
    }

    wchar_t buffer[1024] = { 0 };
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    const LSTATUS status = ::RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size);
    ::RegCloseKey(key);

    if (status != ERROR_SUCCESS || type != REG_SZ) {
        return std::wstring();
    }

    return std::wstring(buffer);
}

//! Verifies the Authenticode signature of the package and, in the same pass,
//! reports the display name of the signing certificate.
//!
//! Signer and validity have to be established together: `WinVerifyTrust` is the
//! only thing that understands every subject type (an MSI keeps its signature in
//! a stream rather than embedded the way a PE does, so parsing the file for a
//! PKCS#7 blob would find nothing).
//!
//! Revocation is not checked: the helper runs unattended and may well have no
//! network by then.
bool verifySignature(const std::wstring& path, std::wstring& signer)
{
    signer.clear();

    WINTRUST_FILE_INFO fileInfo = { };
    fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = path.c_str();

    WINTRUST_DATA data = { };
    data.cbStruct = sizeof(WINTRUST_DATA);
    data.dwUIChoice = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_NONE;
    data.dwUnionChoice = WTD_CHOICE_FILE;
    data.pFile = &fileInfo;
    data.dwStateAction = WTD_STATEACTION_VERIFY;
    data.dwProvFlags = WTD_SAFER_FLAG;

    GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    const LONG status = ::WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &action, &data);

    if (status == ERROR_SUCCESS) {
        CRYPT_PROVIDER_DATA* providerData = ::WTHelperProvDataFromStateData(data.hWVTStateData);
        CRYPT_PROVIDER_SGNR* providerSigner = providerData
                                              ? ::WTHelperGetProvSignerFromChain(providerData, 0, FALSE, 0)
                                              : nullptr;
        CRYPT_PROVIDER_CERT* providerCert = providerSigner
                                            ? ::WTHelperGetProvCertFromChain(providerSigner, 0)
                                            : nullptr;

        if (providerCert && providerCert->pCert) {
            const DWORD size = ::CertGetNameStringW(providerCert->pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,
                                                    nullptr, nullptr, 0);
            if (size > 1) {
                std::vector<wchar_t> name(size);
                ::CertGetNameStringW(providerCert->pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, name.data(), size);
                signer.assign(name.data());
            }
        }
    }

    data.dwStateAction = WTD_STATEACTION_CLOSE;
    ::WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &action, &data);

    return status == ERROR_SUCCESS;
}

bool runProcessAndWait(const std::wstring& application, const std::wstring& commandLine, DWORD& exitCode)
{
    std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back(L'\0');

    STARTUPINFOW startupInfo = { };
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo = { };

    if (!::CreateProcessW(application.c_str(), mutableCommandLine.data(), nullptr, nullptr, FALSE,
                          CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo)) {
        return false;
    }

    ::WaitForSingleObject(processInfo.hProcess, INFINITE);
    ::GetExitCodeProcess(processInfo.hProcess, &exitCode);

    ::CloseHandle(processInfo.hThread);
    ::CloseHandle(processInfo.hProcess);

    return true;
}

bool startProcessDetached(const std::wstring& application, const std::wstring& commandLine)
{
    std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back(L'\0');

    STARTUPINFOW startupInfo = { };
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo = { };

    if (!::CreateProcessW(application.c_str(), mutableCommandLine.data(), nullptr, nullptr, FALSE,
                          DETACHED_PROCESS, nullptr, nullptr, &startupInfo, &processInfo)) {
        return false;
    }

    ::CloseHandle(processInfo.hThread);
    ::CloseHandle(processInfo.hProcess);

    return true;
}

//! The session the application asking for the update is running in - the one
//! its user is looking at, which is not necessarily the console session when
//! more than one is logged on. Has to be asked while that process is still
//! alive; falls back to the console session once it is gone.
DWORD userSessionId(unsigned long long pid)
{
    DWORD sessionId = 0;
    if (pid > 0 && ::ProcessIdToSessionId(static_cast<DWORD>(pid), &sessionId)) {
        return sessionId;
    }

    return ::WTSGetActiveConsoleSessionId();
}

//! We run as SYSTEM, in session 0, where nothing we start would be visible and
//! anything we start would have our privileges. Starts `application` as the
//! user of `sessionId` instead, optionally passing `arguments` and inheriting
//! the handles of this process.
//!
//! `process` receives the handle of the started process when given; the caller
//! then owns it.
bool startInUserSession(const std::wstring& application, const std::wstring& arguments, DWORD sessionId,
                        bool inheritHandles = false, HANDLE* process = nullptr)
{
    if (sessionId == 0xFFFFFFFF) {
        return false;
    }

    HANDLE userToken = nullptr;
    if (!::WTSQueryUserToken(sessionId, &userToken)) {
        return false;
    }

    HANDLE primaryToken = nullptr;
    if (!::DuplicateTokenEx(userToken, MAXIMUM_ALLOWED, nullptr, SecurityImpersonation, TokenPrimary, &primaryToken)) {
        ::CloseHandle(userToken);
        return false;
    }

    void* environment = nullptr;
    const BOOL hasEnvironment = ::CreateEnvironmentBlock(&environment, primaryToken, FALSE);

    std::wstring commandLine = quoted(application);
    if (!arguments.empty()) {
        commandLine += L" " + arguments;
    }

    std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back(L'\0');

    const std::wstring workingDir = parentDir(application);

    STARTUPINFOW startupInfo = { };
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.lpDesktop = const_cast<wchar_t*>(L"winsta0\\default");

    PROCESS_INFORMATION processInfo = { };

    const BOOL ok = ::CreateProcessAsUserW(primaryToken, application.c_str(), mutableCommandLine.data(),
                                           nullptr, nullptr, inheritHandles ? TRUE : FALSE,
                                           CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS,
                                           hasEnvironment ? environment : nullptr,
                                           workingDir.empty() ? nullptr : workingDir.c_str(),
                                           &startupInfo, &processInfo);
    if (ok) {
        ::CloseHandle(processInfo.hThread);

        if (process) {
            *process = processInfo.hProcess;
        } else {
            ::CloseHandle(processInfo.hProcess);
        }
    }

    if (hasEnvironment) {
        ::DestroyEnvironmentBlock(environment);
    }
    ::CloseHandle(primaryToken);
    ::CloseHandle(userToken);

    return ok == TRUE;
}

// ============================================================================
// The progress window
// ============================================================================

//! The half of the progress window that lives on this side: it starts the
//! window as the user - we could not show one ourselves from session 0 - and
//! drives it over a pipe.
//!
//! Nothing here is allowed to fail the update. An installation nobody can watch
//! is worse than one nobody can watch happening.
class ProgressUi
{
public:
    ~ProgressUi()
    {
        stop();
    }

    void start(const shared::UpdateUi& ui, DWORD sessionId)
    {
        if (!ui.isValid()) {
            return;
        }

        SECURITY_ATTRIBUTES attributes = { };
        attributes.nLength = sizeof(attributes);
        attributes.bInheritHandle = TRUE;

        HANDLE readEnd = nullptr;
        HANDLE writeEnd = nullptr;
        if (!::CreatePipe(&readEnd, &writeEnd, &attributes, 64 * 1024)) {
            logLine(L"progress-ui: failed to create the pipe");
            return;
        }

        // Only the read end is the window's to inherit; a write end it could
        // hold open would keep us from ever ending it by closing ours.
        ::SetHandleInformation(writeEnd, HANDLE_FLAG_INHERIT, 0);

        const std::wstring arguments = L"--ui --pipe "
                                       + std::to_wstring(reinterpret_cast<uintptr_t>(readEnd));

        HANDLE process = nullptr;
        const bool started = startInUserSession(modulePath(), arguments, sessionId, /*inheritHandles*/ true, &process);

        ::CloseHandle(readEnd);

        if (!started) {
            logLine(L"progress-ui: failed to start the window in session " + std::to_wstring(sessionId));
            ::CloseHandle(writeEnd);
            return;
        }

        m_write = writeEnd;
        m_process = process;

        // Whatever the application did not send keeps the default of the
        // window rather than being sent as an empty value.
        auto sendIfSet = [this](const char* command, const std::wstring& value) {
            if (!value.empty()) {
                send(command, shared::wideToUtf8(value));
            }
        };

        sendIfSet(updateui::command::TITLE, ui.title);
        sendIfSet(updateui::command::MESSAGE, ui.message);
        sendIfSet(updateui::command::BACKGROUND, ui.backgroundColor);
        sendIfSet(updateui::command::ACCENT, ui.accentColor);
        sendIfSet(updateui::command::FOREGROUND, ui.foregroundColor);

        send(updateui::command::SHOW, std::string());
    }

    //! Percentages of the installation itself; a negative value shows that
    //! something is going on without saying how far along it is.
    void setPercent(int percent)
    {
        if (percent > 100) {
            percent = 100;
        }

        if (percent == m_percent) {
            return;
        }

        m_percent = percent;
        send(updateui::command::PROGRESS, std::to_string(percent));
    }

    void stop()
    {
        //! NOTE: The pipe may already be closed - the window can be closed by
        //! hand - which is no reason to leave the process itself behind.
        if (m_write != INVALID_HANDLE_VALUE) {
            send(updateui::command::CLOSE, std::string());

            ::CloseHandle(m_write);
            m_write = INVALID_HANDLE_VALUE;
        }

        if (m_process) {
            // The window must not outlive the installation it is reporting on.
            if (::WaitForSingleObject(m_process, 5000) != WAIT_OBJECT_0) {
                ::TerminateProcess(m_process, 0);
            }

            ::CloseHandle(m_process);
            m_process = nullptr;
        }
    }

private:
    void send(const char* command, const std::string& argument)
    {
        if (m_write == INVALID_HANDLE_VALUE) {
            return;
        }

        std::string line = command;
        if (!argument.empty()) {
            line += " " + argument;
        }
        line += "\n";

        DWORD written = 0;
        if (!::WriteFile(m_write, line.data(), static_cast<DWORD>(line.size()), &written, nullptr)) {
            // The window is gone; carry on without it.
            ::CloseHandle(m_write);
            m_write = INVALID_HANDLE_VALUE;
        }
    }

    HANDLE m_write = INVALID_HANDLE_VALUE;
    HANDLE m_process = nullptr;
    int m_percent = -1;
};

// ============================================================================
// Task Scheduler
// ============================================================================

class ComScope
{
public:
    ComScope()
    {
        const HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        m_ok = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
        m_needUninitialize = SUCCEEDED(hr);
    }

    ~ComScope()
    {
        if (m_needUninitialize) {
            ::CoUninitialize();
        }
    }

    bool isOk() const { return m_ok; }

private:
    bool m_ok = false;
    bool m_needUninitialize = false;
};

ITaskService* connectTaskService()
{
    ITaskService* service = nullptr;
    HRESULT hr = ::CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService,
                                    reinterpret_cast<void**>(&service));
    if (FAILED(hr) || !service) {
        return nullptr;
    }

    VARIANT empty;
    ::VariantInit(&empty);

    hr = service->Connect(empty, empty, empty, empty);
    if (FAILED(hr)) {
        service->Release();
        return nullptr;
    }

    return service;
}

ITaskFolder* openTaskFolder(ITaskService* service, bool create)
{
    ITaskFolder* rootFolder = nullptr;
    BSTR rootPath = ::SysAllocString(L"\\");
    HRESULT hr = service->GetFolder(rootPath, &rootFolder);
    ::SysFreeString(rootPath);

    if (FAILED(hr) || !rootFolder) {
        return nullptr;
    }

    BSTR folderName = ::SysAllocString(shared::taskFolderName().c_str());

    ITaskFolder* folder = nullptr;
    const std::wstring folderPath = L"\\" + shared::taskFolderName();
    BSTR folderPathStr = ::SysAllocString(folderPath.c_str());
    hr = service->GetFolder(folderPathStr, &folder);
    ::SysFreeString(folderPathStr);

    if (FAILED(hr) && create) {
        VARIANT empty;
        ::VariantInit(&empty);
        hr = rootFolder->CreateFolder(folderName, empty, &folder);
    }

    ::SysFreeString(folderName);
    rootFolder->Release();

    return SUCCEEDED(hr) ? folder : nullptr;
}

struct Registration {
    std::wstring appId;
    std::wstring appExe;        // relative to the install directory
    std::wstring installDir;
    std::wstring packageType;   // "msi" or "exe"
    std::wstring installArgs;
    std::wstring certSubject;   //!< expected signer(s), "|"-separated; usually derived from `certFrom`
    std::wstring certFrom;      //!< signed file - the package being installed - to take the signer from
};

//! An explicit name wins - it is the only way to accept two while a certificate
//! is being rotated; otherwise the signer of the package being installed, so
//! that only whoever signed the application can update it.
std::wstring expectedSigner(const Registration& registration)
{
    if (!registration.certSubject.empty()) {
        return registration.certSubject;
    }

    if (!registration.certFrom.empty()) {
        std::wstring signer;
        if (verifySignature(registration.certFrom, signer) && !signer.empty()) {
            logLine(L"register-task: expected signer taken from " + registration.certFrom + L": " + signer);
            return signer;
        }

        logLine(L"register-task: could not read the signer of " + registration.certFrom);
    }

    //! NOTE: A repair installs from the cached copy of the package, which carries
    //! no signature; keep what an earlier run worked out rather than refuse everything.
    const std::wstring registered = regReadString(shared::registryKeyPath(registration.appId),
                                                  shared::REG_VALUE_CERT_SUBJECT);
    if (!registered.empty()) {
        logLine(L"register-task: keeping the expected signer already registered: " + registered);
    }

    return registered;
}

int registerTask(const Registration& registration)
{
    const std::wstring& appId = registration.appId;
    const std::wstring installDir = trimTrailingSeparators(registration.installDir);

    if (appId.empty() || registration.appExe.empty() || installDir.empty()) {
        logLine(L"register-task: missing --app-id, --app-exe or --install-dir");
        return 1;
    }

    if (registration.packageType != shared::PACKAGE_TYPE_MSI && registration.packageType != shared::PACKAGE_TYPE_EXE) {
        logLine(L"register-task: --package-type must be \"msi\" or \"exe\"");
        return 1;
    }

    // The working area is created up front so that the application can drop a
    // request into it without needing to create anything itself.
    if (!ensureSecureRoot(appId)) {
        logLine(L"register-task: failed to prepare the working directory");
        return 1;
    }

    if (!makeDirectories(shared::requestsDirPath(appId))
        || !secureDirectory(shared::requestsDirPath(appId), REQUESTS_SDDL)) {
        logLine(L"register-task: failed to prepare the requests directory");
        return 1;
    }

    if (!makeDirectories(shared::stagingDirPath(appId))
        || !secureDirectory(shared::stagingDirPath(appId), STAGING_SDDL)) {
        logLine(L"register-task: failed to prepare the staging directory");
        return 1;
    }

    //! NOTE: The absolute path is resolved here, once, rather than composed at
    //! apply time from an assumed layout - the executable sits in a "bin"
    //! subdirectory in some applications and at the root of the installation in
    //! others.
    const std::wstring appPath = installDir + L"\\" + registration.appExe;

    const std::wstring key = shared::registryKeyPath(appId);

    if (!regWriteString(key, shared::REG_VALUE_INSTALL_DIR, installDir)
        || !regWriteString(key, shared::REG_VALUE_APP_PATH, appPath)
        || !regWriteString(key, shared::REG_VALUE_PACKAGE_TYPE, registration.packageType)) {
        logLine(L"register-task: failed to write HKLM values");
        return 1;
    }

    regWriteString(key, shared::REG_VALUE_INSTALL_ARGS, registration.installArgs);

    const std::wstring certSubject = expectedSigner(registration);
    regWriteString(key, shared::REG_VALUE_CERT_SUBJECT, certSubject);

    if (certSubject.empty()) {
        logLine(L"register-task: warning - no expected signer could be established, updates will be refused");
    }

    ComScope com;
    if (!com.isOk()) {
        logLine(L"register-task: failed to initialize COM");
        return 1;
    }

    ITaskService* service = connectTaskService();
    if (!service) {
        logLine(L"register-task: failed to connect to the Task Scheduler");
        return 1;
    }

    ITaskDefinition* definition = nullptr;
    HRESULT hr = service->NewTask(0, &definition);
    if (FAILED(hr) || !definition) {
        service->Release();
        logLine(L"register-task: failed to create a task definition");
        return 1;
    }

    IRegistrationInfo* registrationInfo = nullptr;
    if (SUCCEEDED(definition->get_RegistrationInfo(&registrationInfo)) && registrationInfo) {
        BSTR author = ::SysAllocString(TASK_AUTHOR);
        registrationInfo->put_Author(author);
        ::SysFreeString(author);

        const std::wstring description = L"Installs " + appId + L" updates in the background.";
        BSTR descriptionStr = ::SysAllocString(description.c_str());
        registrationInfo->put_Description(descriptionStr);
        ::SysFreeString(descriptionStr);

        registrationInfo->Release();
    }

    ITaskSettings* settings = nullptr;
    if (SUCCEEDED(definition->get_Settings(&settings)) && settings) {
        settings->put_AllowDemandStart(VARIANT_TRUE);
        settings->put_StartWhenAvailable(VARIANT_FALSE);
        settings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
        settings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
        settings->put_Enabled(VARIANT_TRUE);
        settings->put_Hidden(VARIANT_FALSE);
        settings->put_MultipleInstances(TASK_INSTANCES_IGNORE_NEW);

        BSTR timeLimit = ::SysAllocString(L"PT30M");
        settings->put_ExecutionTimeLimit(timeLimit);
        ::SysFreeString(timeLimit);

        IIdleSettings* idleSettings = nullptr;
        if (SUCCEEDED(settings->get_IdleSettings(&idleSettings)) && idleSettings) {
            idleSettings->put_StopOnIdleEnd(VARIANT_FALSE);
            idleSettings->Release();
        }

        settings->Release();
    }

    IPrincipal* principal = nullptr;
    if (SUCCEEDED(definition->get_Principal(&principal)) && principal) {
        BSTR id = ::SysAllocString(L"Principal");
        principal->put_Id(id);
        ::SysFreeString(id);

        BSTR userId = ::SysAllocString(L"S-1-5-18");
        principal->put_UserId(userId);
        ::SysFreeString(userId);

        principal->put_LogonType(TASK_LOGON_SERVICE_ACCOUNT);
        principal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
        principal->Release();
    }

    //! NOTE: We are that helper, running from the installation the task is being
    //! registered for - no need to guess where it was put.
    const std::wstring helperPath = modulePath();
    if (helperPath.empty()) {
        logLine(L"register-task: failed to determine own path");
        return 1;
    }

    IActionCollection* actions = nullptr;
    if (SUCCEEDED(definition->get_Actions(&actions)) && actions) {
        IAction* action = nullptr;
        if (SUCCEEDED(actions->Create(TASK_ACTION_EXEC, &action)) && action) {
            IExecAction* execAction = nullptr;
            if (SUCCEEDED(action->QueryInterface(IID_IExecAction, reinterpret_cast<void**>(&execAction))) && execAction) {
                BSTR path = ::SysAllocString(helperPath.c_str());
                execAction->put_Path(path);
                ::SysFreeString(path);

                const std::wstring arguments = L"--apply --app-id " + quoted(appId);
                BSTR argumentsStr = ::SysAllocString(arguments.c_str());
                execAction->put_Arguments(argumentsStr);
                ::SysFreeString(argumentsStr);

                const std::wstring workingDir = parentDir(helperPath);
                BSTR workingDirStr = ::SysAllocString(workingDir.c_str());
                execAction->put_WorkingDirectory(workingDirStr);
                ::SysFreeString(workingDirStr);

                execAction->Release();
            }
            action->Release();
        }
        actions->Release();
    }

    ITaskFolder* folder = openTaskFolder(service, /*create*/ true);
    if (!folder) {
        definition->Release();
        service->Release();
        logLine(L"register-task: failed to open or create the task folder");
        return 1;
    }

    VARIANT userId;
    ::VariantInit(&userId);
    userId.vt = VT_BSTR;
    userId.bstrVal = ::SysAllocString(L"S-1-5-18");

    VARIANT password;
    ::VariantInit(&password);

    VARIANT sddl;
    ::VariantInit(&sddl);
    sddl.vt = VT_BSTR;
    sddl.bstrVal = ::SysAllocString(TASK_SDDL);

    BSTR taskNameStr = ::SysAllocString(shared::taskName(appId).c_str());

    IRegisteredTask* registeredTask = nullptr;
    hr = folder->RegisterTaskDefinition(taskNameStr, definition, TASK_CREATE_OR_UPDATE,
                                        userId, password, TASK_LOGON_SERVICE_ACCOUNT, sddl, &registeredTask);

    ::SysFreeString(taskNameStr);
    ::VariantClear(&userId);
    ::VariantClear(&sddl);

    if (registeredTask) {
        registeredTask->Release();
    }

    folder->Release();
    definition->Release();
    service->Release();

    if (FAILED(hr)) {
        logLine(L"register-task: RegisterTaskDefinition failed");
        return 1;
    }

    logLine(L"register-task: registered " + shared::taskPath(appId) + L" -> " + helperPath);
    return 0;
}

int unregisterTask(const std::wstring& appId)
{
    if (appId.empty()) {
        return 1;
    }

    ComScope com;
    if (com.isOk()) {
        ITaskService* service = connectTaskService();
        if (service) {
            ITaskFolder* folder = openTaskFolder(service, /*create*/ false);
            if (folder) {
                BSTR taskNameStr = ::SysAllocString(shared::taskName(appId).c_str());
                folder->DeleteTask(taskNameStr, 0);
                ::SysFreeString(taskNameStr);
                folder->Release();
            }
            service->Release();
        }
    }

    ::RegDeleteKeyExW(HKEY_LOCAL_MACHINE, shared::registryKeyPath(appId).c_str(), KEY_WOW64_64KEY, 0);

    ::DeleteFileW(shared::requestFilePath(appId).c_str());
    ::DeleteFileW(shared::stagedPackagePath(appId, shared::PACKAGE_TYPE_MSI).c_str());
    ::DeleteFileW(shared::stagedPackagePath(appId, shared::PACKAGE_TYPE_EXE).c_str());

    logLine(L"unregister-task: removed " + shared::taskPath(appId));
    return 0;
}

// ============================================================================
// Installing an MSI
// ============================================================================

//! Where the installation has got to, as the Windows Installer reports it.
//!
//! Progress arrives as ticks rather than as a percentage: the engine says how
//! many it expects in total, then counts them off, and an action that turns out
//! to be longer than costed adds to the total as it goes.
struct MsiProgress {
    ProgressUi* ui = nullptr;

    int total = 0;
    int done = 0;

    //! Some actions (removing the previous version, mostly) count down from the
    //! total instead of up from zero.
    bool forward = true;

    //! Actions that report each item they process have a tick value attached
    //! once, before the items themselves start arriving.
    bool actionDataEnabled = false;
    int ticksPerActionData = 0;

    int lastPercent = 0;
};

//! A field the engine did not fill in reads as MSI_NULL_INTEGER, which as a
//! tick count would be a very large negative number.
int recordInteger(MSIHANDLE record, unsigned int field)
{
    const int value = ::MsiRecordGetInteger(record, field);
    return value == MSI_NULL_INTEGER ? 0 : value;
}

void reportProgress(MsiProgress& progress)
{
    if (!progress.ui || progress.total <= 0) {
        return;
    }

    const int done = progress.forward ? progress.done : progress.total - progress.done;

    int percent = static_cast<int>(static_cast<long long>(done) * 100 / progress.total);
    percent = percent < 0 ? 0 : (percent > 99 ? 99 : percent);

    //! NOTE: A major upgrade is several installations in a row, each with a
    //! progress of its own that starts again from nothing. Only ever moving
    //! forward is a better picture of what is happening than a bar that starts
    //! over twice.
    if (percent < progress.lastPercent) {
        return;
    }

    progress.lastPercent = percent;
    progress.ui->setPercent(percent);
}

INT WINAPI msiUiHandler(LPVOID context, UINT messageType, MSIHANDLE record)
{
    MsiProgress* progress = static_cast<MsiProgress*>(context);
    if (!progress) {
        return 0;
    }

    const INSTALLMESSAGE message = static_cast<INSTALLMESSAGE>(0xFF000000 & messageType);

    switch (message) {
    case INSTALLMESSAGE_ACTIONSTART:
        // Whether the action about to run reports its items is said by the
        // action itself, if at all.
        progress->actionDataEnabled = false;
        return IDOK;

    case INSTALLMESSAGE_ACTIONDATA:
        if (progress->actionDataEnabled && progress->ticksPerActionData > 0) {
            progress->done += progress->ticksPerActionData;
            reportProgress(*progress);
        }
        return IDOK;

    case INSTALLMESSAGE_PROGRESS: {
        if (!record) {
            return IDOK;
        }

        switch (recordInteger(record, 1)) {
        case 0: // reset: a new sequence with a total of its own
            progress->total = recordInteger(record, 2);
            progress->forward = recordInteger(record, 3) == 0;
            progress->done = progress->forward ? 0 : progress->total;
            progress->actionDataEnabled = false;
            break;

        case 1: // what one item of the current action is worth
            progress->ticksPerActionData = recordInteger(record, 2);
            progress->actionDataEnabled = recordInteger(record, 3) != 0;
            break;

        case 2: // ticks completed
            progress->done += progress->forward ? recordInteger(record, 2) : -recordInteger(record, 2);
            reportProgress(*progress);
            break;

        case 3: // the action turned out to be bigger than costed
            progress->total += recordInteger(record, 2);
            break;

        default:
            break;
        }

        return IDOK;
    }

    default:
        break;
    }

    return 0;
}

//! Installs `package` through the Windows Installer, reporting progress to
//! `ui`. Returns the ERROR_* code msiexec would have exited with - it is the
//! same engine, driven directly so that its progress can be watched.
UINT installMsi(const std::wstring& package, const std::wstring& properties, ProgressUi& ui)
{
    MsiProgress progress;
    progress.ui = &ui;

    ::MsiSetInternalUI(INSTALLUILEVEL_NONE, nullptr);

    INSTALLUI_HANDLER_RECORD previousHandler = nullptr;
    const DWORD messageFilter = INSTALLLOGMODE_PROGRESS | INSTALLLOGMODE_ACTIONSTART | INSTALLLOGMODE_ACTIONDATA
                                | INSTALLLOGMODE_FATALEXIT | INSTALLLOGMODE_ERROR;

    ::MsiSetExternalUIRecord(msiUiHandler, messageFilter, &progress, &previousHandler);

    const UINT result = ::MsiInstallProductW(package.c_str(), properties.c_str());

    ::MsiSetExternalUIRecord(previousHandler, 0, nullptr, nullptr);

    return result;
}

// ============================================================================
// Applying an update
// ============================================================================

//! The task action. The installer will replace the files of the install
//! location, this image among them, so hand the work over to a copy running
//! from outside it and get out of the way.
int applyDetach(const std::wstring& appId)
{
    const std::wstring self = modulePath();
    const std::wstring detached = shared::detachedHelperPath(appId);

    if (!ensureSecureRoot(appId)) {
        logLine(L"apply: failed to prepare the working directory");
        return 1;
    }

    if (!copyFileWithRetries(self, detached)) {
        logLine(L"apply: failed to copy the helper to " + detached);
        return 1;
    }

    const std::wstring commandLine = quoted(detached) + L" --apply-run --app-id " + quoted(appId);
    if (!startProcessDetached(detached, commandLine)) {
        logLine(L"apply: failed to start the detached helper");
        return 1;
    }

    return 0;
}

int applyRun(const std::wstring& appId)
{
    const std::wstring installDir = regReadString(shared::registryKeyPath(appId), shared::REG_VALUE_INSTALL_DIR);
    const std::wstring appPath = regReadString(shared::registryKeyPath(appId), shared::REG_VALUE_APP_PATH);
    const std::wstring certSubject = regReadString(shared::registryKeyPath(appId), shared::REG_VALUE_CERT_SUBJECT);
    const std::wstring packageType = regReadString(shared::registryKeyPath(appId), shared::REG_VALUE_PACKAGE_TYPE);
    const std::wstring installArgs = regReadString(shared::registryKeyPath(appId), shared::REG_VALUE_INSTALL_ARGS);

    if (installDir.empty() || appPath.empty() || packageType.empty()) {
        logLine(L"apply-run: the HKLM registration is missing or incomplete");
        return 1;
    }

    if (packageType != shared::PACKAGE_TYPE_MSI && packageType != shared::PACKAGE_TYPE_EXE) {
        logLine(L"apply-run: unknown package type \"" + packageType + L"\"");
        return 1;
    }

    shared::UpdateRequest request;
    if (!shared::readRequest(appId, request)) {
        logLine(L"apply-run: no update request found");
        return 1;
    }

    logLine(L"apply-run: request package=" + request.packagePath + L" pid=" + std::to_wstring(request.pid));

    //! NOTE: Asked while the application is still running, and so still has a
    //! session to be asked about.
    const DWORD sessionId = userSessionId(request.pid);

    // 1. Tell the user what is going on. Everything up to the point where the
    //    engine starts costing the package takes an unknown amount of time, so
    //    the window shows that something is happening rather than how far along
    //    it is.
    ProgressUi ui;
    ui.start(request.ui, sessionId);

    // 2. Let the application finish quitting before touching its files.
    if (request.pid > 0) {
        platform::waitForProcessExit(static_cast<long long>(request.pid), /*timeoutMs*/ 60000);
    }

    // 3. Copy the package somewhere an unprivileged user cannot reach, so that
    //    it cannot be swapped after we have verified it.
    const std::wstring staged = shared::stagedPackagePath(appId, packageType);

    if (!makeDirectories(shared::stagingDirPath(appId)) || !secureDirectory(shared::stagingDirPath(appId), STAGING_SDDL)) {
        logLine(L"apply-run: failed to prepare the staging directory");
        return 1;
    }

    ::DeleteFileW(staged.c_str());

    if (!copyFileWithRetries(request.packagePath, staged)) {
        logLine(L"apply-run: failed to copy the package to " + staged);
        return 1;
    }

    // 4. Only now decide whether to trust it.
    auto reject = [&](const std::wstring& reason) {
        logLine(L"apply-run: " + reason);
        ::DeleteFileW(staged.c_str());
        ::DeleteFileW(shared::requestFilePath(appId).c_str());
    };

    //! NOTE: A valid Authenticode signature alone is not enough - the package
    //! path comes from an unprivileged caller, who could otherwise have us
    //! install any signed installer at all. Without a configured signer we have
    //! nothing to compare against, so refuse rather than guess.
    if (certSubject.empty()) {
        reject(L"no expected signer configured, refusing to install");
        return 1;
    }

    std::wstring signer;
    if (!verifySignature(staged, signer)) {
        reject(L"the package is not validly signed, refusing to install it");
        return 1;
    }

    if (!shared::isExpectedSigner(signer, certSubject)) {
        reject(L"unexpected signer \"" + signer + L"\", expected \"" + certSubject + L"\"");
        return 1;
    }

    // 5. Install silently, the way the installer registered.
    //
    //    The extra arguments come from the registration rather than from here:
    //    an MSI needs its install directory passed as a property, an Inno Setup
    //    installer as a switch, and a silent upgrade that is told neither would
    //    relocate an installation the user had put somewhere else.
    const std::wstring extraArgs = shared::expandInstallArgs(installArgs, trimTrailingSeparators(installDir));

    DWORD exitCode = 0;

    if (packageType == shared::PACKAGE_TYPE_MSI) {
        //! NOTE: The engine is driven in-process rather than by starting
        //! msiexec.exe, which is the same installation either way - but only
        //! this way does it report what it is doing, which is what the progress
        //! bar shows. `REBOOT=ReallySuppress` is what `/norestart` sets, and
        //! the user interface level takes the place of `/qn`.
        std::wstring properties = L"REBOOT=ReallySuppress";
        if (!extraArgs.empty()) {
            properties += L" " + extraArgs;
        }

        logLine(L"apply-run: installing " + staged + L" with " + properties);

        //! NOTE: The window keeps its marquee until the engine has costed the
        //! package and says something; an empty bar sitting at nothing for the
        //! first few seconds would look stuck.
        exitCode = installMsi(staged, properties, ui);
    } else {
        //! NOTE: A self-contained installer reports nothing we could read, so
        //! the window keeps saying only that the installation is under way.
        std::wstring commandLine = quoted(staged);
        if (!extraArgs.empty()) {
            commandLine += L" " + extraArgs;
        }

        logLine(L"apply-run: running " + commandLine);

        if (!runProcessAndWait(staged, commandLine, exitCode)) {
            logLine(L"apply-run: failed to start the installer");
            ui.stop();
            return 1;
        }
    }

    // ERROR_SUCCESS_REBOOT_INITIATED (1641) and ERROR_SUCCESS_REBOOT_REQUIRED (3010)
    // both mean the installation itself succeeded.
    const bool installed = exitCode == 0 || exitCode == 1641 || exitCode == 3010;
    if (!installed) {
        logLine(L"apply-run: the installer failed with exit code " + std::to_wstring(exitCode));
        ::DeleteFileW(shared::requestFilePath(appId).c_str());
        ui.stop();
        return 1;
    }

    ui.setPercent(100);

    logLine(L"apply-run: installed successfully, exit code " + std::to_wstring(exitCode));

    // 6. Clean up before relaunching; the request must not survive to be
    //    replayed on the next run.
    ::DeleteFileW(shared::requestFilePath(appId).c_str());
    ::DeleteFileW(staged.c_str());
    ::DeleteFileW(request.packagePath.c_str());

    // 7. Take the window down before the application it was standing in for
    //    comes back up.
    ui.stop();

    // 8. Bring the application back, as the user rather than as us.
    if (!startInUserSession(appPath, std::wstring(), sessionId)) {
        logLine(L"apply-run: failed to relaunch " + appPath);
        // The update itself succeeded; the user can start the application manually.
    }

    return 0;
}

std::map<std::wstring, std::wstring> parseArguments(const std::vector<std::wstring>& arguments)
{
    std::map<std::wstring, std::wstring> result;

    for (size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i].rfind(L"--", 0) != 0) {
            continue;
        }

        if (i + 1 < arguments.size() && arguments[i + 1].rfind(L"--", 0) != 0) {
            result[arguments[i]] = arguments[i + 1];
            ++i;
        } else {
            result[arguments[i]] = std::wstring();
        }
    }

    return result;
}

std::wstring valueOf(const std::map<std::wstring, std::wstring>& arguments, const wchar_t* key)
{
    const auto it = arguments.find(key);
    return it != arguments.end() ? it->second : std::wstring();
}
}

namespace updatetask {
int runCommandLine()
{
    int argc = 0;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (!argv) {
        return 1;
    }

    std::vector<std::wstring> arguments;
    for (int i = 1; i < argc; ++i) {
        arguments.emplace_back(argv[i]);
    }
    ::LocalFree(argv);

    const std::map<std::wstring, std::wstring> parsed = parseArguments(arguments);

    //! NOTE: Handled before everything else: this is the one command that does
    //! not run privileged, has no working directory to secure and no business
    //! writing to a log only SYSTEM can open.
    if (parsed.count(L"--ui") > 0) {
        return updateui::run(valueOf(parsed, L"--pipe"));
    }

    const bool isRegister = parsed.count(L"--register-task") > 0;
    const bool isUnregister = parsed.count(L"--unregister-task") > 0;
    const bool isApply = parsed.count(L"--apply") > 0;
    const bool isApplyRun = parsed.count(L"--apply-run") > 0;

    if (!isRegister && !isUnregister && !isApply && !isApplyRun) {
        return 1;
    }

    const std::wstring appId = valueOf(parsed, L"--app-id");
    if (appId.empty()) {
        return 1;
    }

    //! NOTE: Secured before the log is opened - every one of these commands runs
    //! privileged, and the log lives in that same directory.
    ensureSecureRoot(appId);
    openLog(appId);

    int returnCode = 1;

    if (isRegister) {
        Registration registration;
        registration.appId = appId;
        registration.appExe = valueOf(parsed, L"--app-exe");
        registration.installDir = valueOf(parsed, L"--install-dir");
        registration.packageType = parsed.count(L"--package-type") ? valueOf(parsed, L"--package-type")
                                   : std::wstring(shared::PACKAGE_TYPE_MSI);
        registration.installArgs = valueOf(parsed, L"--install-args");
        registration.certSubject = valueOf(parsed, L"--cert-subject");
        registration.certFrom = valueOf(parsed, L"--cert-from");

        returnCode = registerTask(registration);
    } else if (isUnregister) {
        returnCode = unregisterTask(appId);
    } else if (isApply) {
        returnCode = applyDetach(appId);
    } else {
        returnCode = applyRun(appId);
    }

    closeLog();

    return returnCode;
}
}
