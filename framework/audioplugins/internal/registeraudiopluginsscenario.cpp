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

#include "registeraudiopluginsscenario.h"

#include <QCoreApplication>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "global/containers.h"
#include "global/translation.h"

#include "audiopluginserrors.h"

#include "log.h"

using namespace muse;
using namespace muse::audioplugins;

#ifdef MUSE_MODULE_AUDIOPLUGINS_SCAN_TRACE
#define SCAN_TRACE() LOGI()
#else
#define SCAN_TRACE() LOGN()
#endif

namespace {
// completeBasename() is empty for LV2 "<uri>@<bundle>/" composites and
// load() rejects empty ids; fall back to the full path.
std::string placeholderIdFromPath(const io::path_t& path)
{
    std::string id = io::completeBasename(path).toStdString();
    if (id.empty()) {
        id = path.toStdString();
    }
    return id;
}

int64_t pluginScanConcurrency()
{
    const unsigned int concurrency = std::thread::hardware_concurrency();
    return concurrency > 0 ? static_cast<int64_t>(concurrency) : 1;
}

void processProgressEvents()
{
    if (QCoreApplication::instance()) {
        QCoreApplication::processEvents();
    }
}

constexpr int AUDIO_PLUGIN_REGISTRATION_TIMEOUT_MS = 15000;
}

void RegisterAudioPluginsScenario::init()
{
    TRACEFUNC;

    m_progress.canceled().onNotify(this, [this]() {
        SCAN_TRACE() << "Audio plugin scan cancellation requested";
        m_aborted = true;
    });

    Ret ret = knownPluginsRegister()->load();
    if (!ret) {
        LOGE() << ret.toString();
    }
}

Ret RegisterAudioPluginsScenario::markCrashedPluginsAsBroken()
{
    TRACEFUNC;

    const PluginResourceIdList crashedIds = loadGuard()->danglingLoads();
    if (crashedIds.empty()) {
        return make_ok();
    }

    AudioPluginInfoList brokenInfos;

    for (const AudioPluginInfo& info : knownPluginsRegister()->pluginInfoList()) {
        if (!muse::contains(crashedIds, info.meta.id)) {
            continue;
        }

        LOGI() << "Plugin crashed the application while loading in a previous run, marking as broken: " << info.meta.id;

        AudioPluginInfo broken = info;
        broken.state = AudioPluginState::Error;
        broken.errorCode = static_cast<int>(Err::PluginCrashedOnLoad);
        brokenInfos.push_back(std::move(broken));
    }

    Ret ret = make_ok();

    if (!brokenInfos.empty()) {
        PluginResourceIdList brokenIds;
        brokenIds.reserve(brokenInfos.size());
        for (const AudioPluginInfo& info : brokenInfos) {
            brokenIds.push_back(info.meta.id);
        }

        ret = knownPluginsRegister()->unregisterPlugins(brokenIds);
        if (ret) {
            ret = knownPluginsRegister()->registerPlugins(brokenInfos);
        }

        if (!ret) {
            LOGE() << "Failed to mark crashed plugins as broken: " << ret.toString();
            return ret;
        }
    }

    return loadGuard()->clearDanglingLoads();
}

PluginScanResult RegisterAudioPluginsScenario::scanPlugins(Progress* progress) const
{
    TRACEFUNC;

    PluginScanResult result;

    // one binary path can host several plugin ids (shell bundles)
    std::map<io::path_t, std::vector<AudioPluginState> > registered;
    for (const auto& info : knownPluginsRegister()->pluginInfoList()) {
        registered[info.path].push_back(info.state);
    }

    for (const auto& scanner : scannerRegister()->scanners()) {
        for (const auto& path : scanner->scanPlugins(progress)) {
            auto it = registered.find(path);
            if (it == registered.end()) {
                result.newPluginPaths.push_back(path);
                continue;
            }

            const std::vector<AudioPluginState>& states = it->second;

            // Discovered placeholder: a prior run was interrupted, re-validate
            const bool hasDiscovered = std::any_of(states.cbegin(), states.cend(),
                                                   [](AudioPluginState state) {
                return state == AudioPluginState::Discovered;
            });
            if (hasDiscovered) {
                result.newPluginPaths.push_back(path);
                registered.erase(it);
                continue;
            }

            // a Missing path is back: re-validate rather than trust the cache,
            // the binary may have changed
            const bool hasMissing = std::any_of(states.cbegin(), states.cend(),
                                                [](AudioPluginState state) {
                return state == AudioPluginState::Missing;
            });
            if (hasMissing) {
                result.newPluginPaths.push_back(path);
            }
            registered.erase(it);
        }
    }

    // paths no scanner reports anymore are missing
    for (const auto& [path, states] : registered) {
        // skip paths already fully Missing, nothing to transition
        const bool hasTransition = std::any_of(states.cbegin(), states.cend(),
                                               [](AudioPluginState state) {
            return state != AudioPluginState::Missing;
        });
        if (hasTransition) {
            result.missingPluginPaths.push_back(path);
        }
    }

    return result;
}

Ret RegisterAudioPluginsScenario::rescanAllPlugins()
{
    TRACEFUNC;

    Ret ret = knownPluginsRegister()->clear();
    if (!ret) {
        LOGE() << "Failed to clear plugins registry: " << ret.toString();
        return ret;
    }

    return updatePluginsRegistry();
}

Ret RegisterAudioPluginsScenario::updatePluginsRegistry()
{
    TRACEFUNC;

    PluginScanResult result = scanPlugins();

    Ret ret = knownPluginsRegister()->setPluginsState(result.missingPluginPaths, AudioPluginState::Missing);
    if (!ret) {
        LOGE() << "Failed to mark missing plugins: " << ret.toString();
        return ret;
    }

    ret = registerNewPlugins(result.newPluginPaths, /*validate*/ true);
    if (!ret) {
        LOGE() << "Failed to register new plugins: " << ret.toString();
        return ret;
    }

    return knownPluginsRegister()->load();
}

Ret RegisterAudioPluginsScenario::registerNewPlugins(const io::paths_t& pluginPaths, bool validate)
{
    TRACEFUNC;

    if (pluginPaths.empty()) {
        return make_ok();
    }

    Ret ret = persistDiscoveredPlaceholders(pluginPaths);
    if (!ret) {
        return ret;
    }

    if (validate) {
        SCAN_TRACE() << "Starting audio plugin validation for " << pluginPaths.size() << " plugin paths";
        processPluginsRegistration(pluginPaths);
    }

    return knownPluginsRegister()->load();
}

Ret RegisterAudioPluginsScenario::persistDiscoveredPlaceholders(const io::paths_t& pluginPaths)
{
    // Placeholders make scanPlugins() re-validate these paths next launch.
    // Clear any prior entry at the path first to avoid the same-id-same-path assert.
    AudioPluginInfoList placeholders;
    placeholders.reserve(pluginPaths.size());
    for (const io::path_t& path : pluginPaths) {
        Ret ret = knownPluginsRegister()->removePluginsAtPath(path);
        if (!ret) {
            return ret;
        }

        AudioPluginInfo info;
        info.meta.id = placeholderIdFromPath(path);
        info.meta.type = metaType(path);
        info.path = path;
        info.state = AudioPluginState::Discovered;
        placeholders.emplace_back(std::move(info));
    }
    return knownPluginsRegister()->registerPlugins(placeholders);
}

Ret RegisterAudioPluginsScenario::unregisterRemovedPlugins(const PluginResourceIdList& pluginIds)
{
    TRACEFUNC;

    if (pluginIds.empty()) {
        return make_ok();
    }

    Ret ret = knownPluginsRegister()->unregisterPlugins(pluginIds);
    if (!ret) {
        LOGE() << "Failed to unregister removed plugins: " << ret.toString();
    }

    return ret;
}

AudioPluginInfoList RegisterAudioPluginsScenario::scanResult(const io::path_t& pluginPath, const io::path_t& resultFile, int code) const
{
    if (code == 0) {
        RetVal<AudioPluginInfoList> res = knownPluginsRegister()->readPluginsFrom(resultFile);
        if (res.ret) {
            fileSystem()->remove(resultFile);
            return res.val;
        } else {
            LOGE() << "Could not read scan result for " << pluginPath.toStdString() << ": " << res.ret.toString();
        }
    } else {
        LOGE() << "Could not register plugin: " << pluginPath.toStdString() << "\n error code: " << code;
    }

    fileSystem()->remove(resultFile);
    return { makeFailedPluginInfo(pluginPath, code) };
}

static void appendPluginInfos(AudioPluginInfoList& destination, const AudioPluginInfoList& source)
{
    destination.insert(destination.end(), source.cbegin(), source.cend());
}

void RegisterAudioPluginsScenario::processPluginsRegistration(const io::paths_t& pluginPaths)
{
    interactive()->showProgress(muse::trc("audio", "Validating audio plugins"), m_progress);

    m_aborted = false;
    m_progress.start();
    processProgressEvents();

    const std::string appPath = globalConfiguration()->appBinPath().toStdString();
    const int64_t pluginCount = static_cast<int64_t>(pluginPaths.size());
    if (pluginCount == 0) {
        m_progress.finish(muse::make_ok());
        return;
    }

    const int64_t concurrency = pluginScanConcurrency();
    const size_t registryFlushSize = 64;

    SCAN_TRACE() << "Audio plugin scan process started: pluginCount=" << pluginCount
                 << ", concurrency=" << concurrency
                 << ", timeoutMs=" << AUDIO_PLUGIN_REGISTRATION_TIMEOUT_MS
                 << ", appPath=" << appPath;

    struct CompletedScan {
        int64_t index = 0;
        io::path_t resultFile;
        int code = 0;
    };

    std::mutex completedMutex;
    std::condition_variable completedChanged;
    std::vector<CompletedScan> completedScans;
    std::atomic<int64_t> nextIndex { 0 };
    std::atomic<int64_t> activeWorkers { 0 };
    std::vector<std::thread> workers;
    PluginResourceIdList completedPlaceholderIds;
    AudioPluginInfoList completedPluginInfo;
    int64_t doneCount = 0;

    auto flushCompletedScanResults = [&]() {
        if (completedPlaceholderIds.empty() && completedPluginInfo.empty()) {
            return;
        }

        SCAN_TRACE() << "Flushing audio plugin scan results: placeholders=" << completedPlaceholderIds.size()
                     << ", pluginInfos=" << completedPluginInfo.size();

        Ret ret = knownPluginsRegister()->unregisterPlugins(completedPlaceholderIds);
        if (!ret) {
            LOGE() << "Failed to remove completed plugin placeholders: " << ret.toString();
        }

        ret = knownPluginsRegister()->registerPlugins(completedPluginInfo);
        if (!ret) {
            LOGE() << "Failed to register scanned plugins: " << ret.toString();
        }

        completedPlaceholderIds.clear();
        completedPluginInfo.clear();
    };

    auto worker = [&](int64_t workerId) {
        SCAN_TRACE() << "Audio plugin scan worker started: workerId=" << workerId;
        while (!m_aborted.load() && !m_progress.isCanceled()) {
            const int64_t index = nextIndex.fetch_add(1);
            if (index >= pluginCount) {
                break;
            }

            const io::path_t resultFile = scanResultFilePath(index);
            const std::string pluginPathStr = pluginPaths[index].toStdString();

            SCAN_TRACE() << "Audio plugin scan worker " << workerId
                         << " validating index=" << index << "/" << pluginCount
                         << ", resultFile=" << resultFile.toStdString()
                         << ", pluginPath=" << pluginPathStr;

            // clear leftovers from a previous run
            fileSystem()->remove(resultFile);

            const int code = process()->execute(appPath,
                                                { "--register-audio-plugin", pluginPathStr, "--register-audio-plugin-out",
                                                  resultFile.toStdString() },
                                                AUDIO_PLUGIN_REGISTRATION_TIMEOUT_MS,
                                                [this]() { return m_aborted.load() || m_progress.isCanceled(); });

            SCAN_TRACE() << "Audio plugin scan worker " << workerId
                         << " validation finished: index=" << index
                         << ", code=" << code
                         << ", aborted=" << m_aborted.load()
                         << ", pluginPath=" << pluginPathStr;

            {
                std::lock_guard lock(completedMutex);
                completedScans.push_back({ index, resultFile, code });
            }
            completedChanged.notify_one();
        }

        const int64_t remainingWorkers = --activeWorkers;
        SCAN_TRACE() << "Audio plugin scan worker stopped: workerId=" << workerId
                     << ", remainingWorkers=" << remainingWorkers
                     << ", aborted=" << m_aborted.load();
        completedChanged.notify_one();
    };

    const int64_t workerCount = std::min(concurrency, pluginCount);
    activeWorkers = workerCount;
    workers.reserve(static_cast<size_t>(workerCount));
    for (int64_t i = 0; i < workerCount; ++i) {
        workers.emplace_back(worker, i);
    }

    while (doneCount < pluginCount) {
        CompletedScan scan;
        {
            std::unique_lock lock(completedMutex);
            while (completedScans.empty() && activeWorkers.load() > 0) {
                completedChanged.wait_for(lock, std::chrono::milliseconds(50));
                lock.unlock();
                processProgressEvents();
                lock.lock();
            }

            if (completedScans.empty()) {
                SCAN_TRACE() << "Audio plugin scan completion loop ended with no completed scans: doneCount=" << doneCount
                             << ", pluginCount=" << pluginCount
                             << ", activeWorkers=" << activeWorkers.load()
                             << ", aborted=" << m_aborted.load();
                break;
            }

            scan = completedScans.back();
            completedScans.pop_back();
        }

        ++doneCount;
        SCAN_TRACE() << "Audio plugin scan result received: doneCount=" << doneCount << "/" << pluginCount
                     << ", index=" << scan.index
                     << ", code=" << scan.code
                     << ", resultFile=" << scan.resultFile.toStdString()
                     << ", pluginPath=" << pluginPaths[scan.index].toStdString();

        if (scan.code == IProcess::ExecuteCanceledCode) {
            SCAN_TRACE() << "Audio plugin scan result ignored after cancellation: index=" << scan.index
                         << ", pluginPath=" << pluginPaths[scan.index].toStdString();
            continue;
        }

        completedPlaceholderIds.push_back(placeholderIdFromPath(pluginPaths[scan.index]));
        appendPluginInfos(completedPluginInfo, scanResult(pluginPaths[scan.index], scan.resultFile, scan.code));

        m_progress.progress(doneCount, pluginCount, io::filename(pluginPaths[scan.index]).toStdString());
        processProgressEvents();

        if (completedPlaceholderIds.size() >= registryFlushSize) {
            flushCompletedScanResults();
        }

        if (m_aborted.load() && activeWorkers.load() == 0) {
            break;
        }
    }

    for (std::thread& workerThread : workers) {
        if (workerThread.joinable()) {
            SCAN_TRACE() << "Joining audio plugin scan worker thread";
            workerThread.join();
        }
    }

    flushCompletedScanResults();

    SCAN_TRACE() << "Audio plugin scan process finished: doneCount=" << doneCount
                 << ", pluginCount=" << pluginCount
                 << ", activeWorkers=" << activeWorkers.load()
                 << ", aborted=" << m_aborted.load();

    if (!m_aborted.load()) {
        m_progress.finish(muse::make_ok());
    }
    processProgressEvents();
}

RetVal<AudioPluginInfoList> RegisterAudioPluginsScenario::validatePluginInfo(const io::path_t& pluginPath) const
{
    SCAN_TRACE() << "Validating audio plugin metadata: pluginPath=" << pluginPath.toStdString();

    const IAudioPluginMetaReaderPtr reader = metaReader(pluginPath);
    if (!reader) {
        SCAN_TRACE() << "No audio plugin meta reader found: pluginPath=" << pluginPath.toStdString();
        return RetVal<AudioPluginInfoList>(make_ret(Err::UnknownPluginType));
    }

    const RetVal<PluginMetaList> metaList = reader->readMeta(pluginPath);
    if (!metaList.ret) {
        SCAN_TRACE() << "Failed reading audio plugin metadata: pluginPath=" << pluginPath.toStdString()
                     << ", ret=" << metaList.ret.toString();
        return RetVal<AudioPluginInfoList>(metaList.ret);
    }

    SCAN_TRACE() << "Audio plugin metadata read: pluginPath=" << pluginPath.toStdString()
                 << ", metaCount=" << metaList.val.size();

    AudioPluginInfoList infoList;
    infoList.reserve(metaList.val.size());

    for (const PluginMeta& meta : metaList.val) {
        AudioPluginInfo info;
        info.meta = meta;
        info.path = pluginPath;
        info.state = AudioPluginState::Validated;
        infoList.emplace_back(std::move(info));
    }

    return RetVal<AudioPluginInfoList>::make_ok(infoList);
}

Ret RegisterAudioPluginsScenario::registerPlugin(const io::path_t& pluginPath)
{
    TRACEFUNC;

    IF_ASSERT_FAILED(!pluginPath.empty()) {
        return false;
    }

    Ret ret = knownPluginsRegister()->removePluginsAtPath(pluginPath);
    if (!ret) {
        LOGE() << "Failed to clear existing entry at " << pluginPath.toStdString()
               << ": " << ret.toString();
        return ret;
    }

    RetVal<AudioPluginInfoList> infoList = validatePluginInfo(pluginPath);
    if (!infoList.ret) {
        LOGE() << infoList.ret.toString();
        return infoList.ret;
    }

    return knownPluginsRegister()->registerPlugins(infoList.val);
}

Ret RegisterAudioPluginsScenario::validatePlugin(const io::path_t& pluginPath, const io::path_t& outputFile)
{
    TRACEFUNC;

    IF_ASSERT_FAILED(!pluginPath.empty()) {
        return false;
    }

    SCAN_TRACE() << "Audio plugin validation subprocess started: pluginPath=" << pluginPath.toStdString()
                 << ", outputFile=" << outputFile.toStdString();

    RetVal<AudioPluginInfoList> infoList = validatePluginInfo(pluginPath);
    if (!infoList.ret) {
        LOGE() << infoList.ret.toString();
        return infoList.ret;
    }

    Ret ret = knownPluginsRegister()->writePluginsTo(outputFile, infoList.val);
    SCAN_TRACE() << "Audio plugin validation subprocess finished: pluginPath=" << pluginPath.toStdString()
                 << ", outputFile=" << outputFile.toStdString()
                 << ", pluginInfoCount=" << infoList.val.size()
                 << ", ret=" << ret.toString();
    return ret;
}

AudioPluginInfo RegisterAudioPluginsScenario::makeFailedPluginInfo(const io::path_t& pluginPath, int failCode) const
{
    AudioPluginInfo info;
    info.meta.id = placeholderIdFromPath(pluginPath);
    info.meta.type = metaType(pluginPath);
    info.path = pluginPath;
    info.state = AudioPluginState::Error;
    info.errorCode = failCode != 0 ? failCode : -1;
    return info;
}

io::path_t RegisterAudioPluginsScenario::scanResultFilePath(int64_t index) const
{
    return fileSystem()->temporaryDirectoryPath() + "/muse_audioplugin_scan_" + std::to_string(index) + ".json";
}

IAudioPluginMetaReaderPtr RegisterAudioPluginsScenario::metaReader(const io::path_t& pluginPath) const
{
    for (const IAudioPluginMetaReaderPtr& reader : metaReaderRegister()->readers()) {
        if (reader->canReadMeta(pluginPath)) {
            return reader;
        }
    }

    return nullptr;
}

audioplugins::PluginType RegisterAudioPluginsScenario::metaType(const io::path_t& pluginPath) const
{
    const IAudioPluginMetaReaderPtr reader = metaReader(pluginPath);
    return reader ? reader->metaType() : audioplugins::PluginType();
}
