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
#include "winupdateinstaller.h"

//! NOTE: Qt first - <windows.h> defines min/max and a pile of A/W macros that
//! break headers included after it.
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <taskschd.h>

#include "winupdateshared.h"

#include "../../../updateerrors.h"

#include "log.h"

using namespace muse;
using namespace muse::update;

namespace {
//! COM may or may not already be initialised on the calling thread; initialise
//! it if needed and undo only what we did ourselves.
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

//! Returns the task registered by the installer, or nullptr. The caller owns the
//! returned interface.
IRegisteredTask* openUpdateTask(const std::wstring& appId)
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

    const std::wstring folderPath = L"\\" + win::taskFolderName();
    BSTR folderPathStr = ::SysAllocString(folderPath.c_str());

    ITaskFolder* folder = nullptr;
    hr = service->GetFolder(folderPathStr, &folder);
    ::SysFreeString(folderPathStr);
    service->Release();

    if (FAILED(hr) || !folder) {
        return nullptr;
    }

    const std::wstring name = win::taskName(appId);
    BSTR nameStr = ::SysAllocString(name.c_str());

    IRegisteredTask* task = nullptr;
    hr = folder->GetTask(nameStr, &task);
    ::SysFreeString(nameStr);
    folder->Release();

    if (FAILED(hr)) {
        return nullptr;
    }

    return task;
}

bool isTaskEnabled(IRegisteredTask* task)
{
    VARIANT_BOOL enabled = VARIANT_FALSE;
    return SUCCEEDED(task->get_Enabled(&enabled)) && enabled != VARIANT_FALSE;
}

QString registeredValue(const std::wstring& appId, const wchar_t* name)
{
    HKEY key = nullptr;
    if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, muse::update::win::registryKeyPath(appId).c_str(), 0,
                        KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        return QString();
    }

    wchar_t buffer[1024] = { 0 };
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    const LSTATUS status = ::RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size);
    ::RegCloseKey(key);

    if (status != ERROR_SUCCESS || type != REG_SZ) {
        return QString();
    }

    return QString::fromWCharArray(buffer);
}

//! Whether the task belongs to the copy of the application that is running.
//!
//! The registered executable is compared directly rather than derived from the
//! install directory: where the executable sits inside an installation differs
//! between applications, and a portable copy running next to an installed one
//! would otherwise hand its package to the task and update the *other* one.
bool isRegisteredForThisInstall(const std::wstring& appId)
{
    const QString registered = registeredValue(appId, muse::update::win::REG_VALUE_APP_PATH);
    if (registered.isEmpty()) {
        return false;
    }

    const QFileInfo registeredInfo(registered);
    const QFileInfo runningInfo(QCoreApplication::applicationFilePath());

    if (!registeredInfo.exists()) {
        return false;
    }

    return registeredInfo.canonicalFilePath().compare(runningInfo.canonicalFilePath(), Qt::CaseInsensitive) == 0;
}
}

std::wstring WinUpdateInstaller::appId() const
{
    const QString baseName = QFileInfo(QCoreApplication::applicationFilePath()).completeBaseName();
    return baseName.toStdWString();
}

bool WinUpdateInstaller::isInPlaceUpdateSupported() const
{
    if (!isRegisteredForThisInstall(appId())) {
        return false;
    }

    ComScope com;
    if (!com.isOk()) {
        return false;
    }

    IRegisteredTask* task = openUpdateTask(appId());
    if (!task) {
        return false;
    }

    const bool enabled = isTaskEnabled(task);
    task->Release();

    return enabled;
}

Ret WinUpdateInstaller::applyUpdate(const muse::io::path_t& packagePath)
{
    if (!fileSystem()->exists(packagePath)) {
        LOGE() << "update package does not exist: " << packagePath;
        return make_ret(Err::UnknownError);
    }

    ComScope com;
    if (!com.isOk()) {
        LOGE() << "failed to initialize COM";
        return make_ret(Err::UnknownError);
    }

    const std::wstring id = appId();

    if (!isRegisteredForThisInstall(id)) {
        LOGE() << "the update task is registered for a different installation";
        return make_ret(Err::UnknownError);
    }

    IRegisteredTask* task = openUpdateTask(id);
    if (!task) {
        LOGE() << "update task is not registered";
        return make_ret(Err::UnknownError);
    }

    if (!isTaskEnabled(task)) {
        LOGE() << "update task is disabled";
        task->Release();
        return make_ret(Err::UnknownError);
    }

    //! NOTE: An absolute native path; the helper reads it as untrusted input and
    //! re-verifies the package signature after copying it out of reach.
    const QString nativePath = QDir::toNativeSeparators(QFileInfo(packagePath.toQString()).absoluteFilePath());

    win::UpdateRequest request;
    request.packagePath = nativePath.toStdWString();
    request.pid = static_cast<unsigned long long>(QCoreApplication::applicationPid());

    if (!win::writeRequest(id, request)) {
        LOGE() << "failed to write update request to " << QString::fromStdWString(win::requestFilePath(id));
        task->Release();
        return make_ret(Err::UnknownError);
    }

    VARIANT empty;
    ::VariantInit(&empty);

    IRunningTask* runningTask = nullptr;
    const HRESULT hr = task->Run(empty, &runningTask);
    if (runningTask) {
        runningTask->Release();
    }
    task->Release();

    if (FAILED(hr)) {
        LOGE() << "failed to run update task, hr=" << static_cast<int>(hr);
        ::DeleteFileW(win::requestFilePath(id).c_str());
        return make_ret(Err::UnknownError);
    }

    LOGI() << "update task started, it will install " << nativePath << " once this process quits";
    return make_ok();
}
