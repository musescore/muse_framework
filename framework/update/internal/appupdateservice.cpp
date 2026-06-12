/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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

#include "appupdateservice.h"

#include <QBuffer>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrlQuery>

#include "update/updateerrors.h"

#include "downloadfiledevice.h"

#include "defer.h"
#include "translation.h"
#include "log.h"

using namespace muse;
using namespace muse::update;
using namespace muse::network;
using namespace muse::async;
using namespace muse::io;

const QString INSTALLED_WEEK_BEGINNING_KEY("Installed-Week-Beginning");
const QString PREVIOUS_REQUEST_DAY_KEY("Previous-Request-Day");

static const std::string PARTIAL_SUFFIX(".part");

static QDate calculateWeekBeginForDate(const QDate& date)
{
    // 1 (Monday) + 6 mod 7 = 0
    // 2 (Tuesday) + 6 mod 7 = 1
    // etc...
    int diffToWeekBegin = (date.dayOfWeek() + 6) % 7;
    return QDate::currentDate().addDays(-diffToWeekBegin);
}

static ISystemInfo::CpuArchitecture assetArch(const QString& asset)
{
    if (asset.contains("aarch64")) {
        return ISystemInfo::CpuArchitecture::Arm64;
    } else if (asset.contains("arm")) {
        return ISystemInfo::CpuArchitecture::Arm;
    }

    return ISystemInfo::CpuArchitecture::x86_64;
}

static PrevReleasesNotesList parsePreviousReleasesNotes(const QByteArray& json)
{
    PrevReleasesNotesList result;

    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        LOGE() << "failed parse, err: " << err.errorString();
        return result;
    }

    QJsonObject jsonObject = jsonDoc.object();

    if (jsonObject.empty()) {
        LOGE() << "failed parse, no jsonObject";
        return result;
    }

    QJsonArray releases = jsonObject.value("releases").toArray();
    for (const QJsonValue& release : releases) {
        QJsonObject releaseObj = release.toObject();
        std::string version = releaseObj.value("version").toString().toStdString();

        result.push_back({ std::move(version), releaseObj.value("notes").toString().toStdString() });
    }

    return result;
}

void AppUpdateService::init()
{
    TRACEFUNC;

    io::path_t historyPath = configuration()->updateRequestHistoryJsonPath();
    if (fileSystem()->exists(historyPath)) {
        return;
    }

    // If the request history file doesn't exist, perform a first time setup using today's date...
    UpdateRequestHistory updateRequestHistory;
    updateRequestHistory.installedWeekBeginning = calculateWeekBeginForDate(QDate::currentDate());
    updateRequestHistory.previousRequestDay = QDate::currentDate();

    Ret ret = writeUpdateRequestHistory(historyPath, updateRequestHistory);
    if (!ret) {
        LOGE() << ret.toString();
    }
}

Promise<RetVal<ReleaseInfo> > AppUpdateService::checkForUpdate()
{
    return Promise<RetVal<ReleaseInfo> >([this](auto resolve, auto) {
        clear();

        if (configuration()->checkForUpdateTestMode()) {
            return resolve(m_lastCheckResult);
        }

        if (!m_networkManager) {
            m_networkManager = networkManagerCreator()->makeNetworkManager();
        }

        // Read request history file...
        io::path_t historyPath = configuration()->updateRequestHistoryJsonPath();
        RetVal<UpdateRequestHistory> historyRv = readUpdateRequestHistory(historyPath);
        if (!historyRv.ret) {
            LOGE() << historyRv.ret.toString();
        }

        QUrl url = QString::fromStdString(configuration()->checkForAppUpdateUrl());
        if (requestParamsProvider()) {
            QUrlQuery query(url);
            for (const auto& [key, value] : requestParamsProvider()->updateRequestParams()) {
                query.addQueryItem(QString::fromStdString(key), QString::fromStdString(value));
            }

            if (!query.isEmpty()) {
                url.setQuery(query);
            }
        }

        RequestHeaders headers = prepareHeaders(historyRv.val);
        auto buff = std::make_shared<QBuffer>();

        RetVal<Progress> progress = m_networkManager->get(url, buff, headers);
        if (!progress.ret) {
            m_lastCheckResult.ret = progress.ret;
            return resolve(m_lastCheckResult);
        }

        progress.val.finished().onReceive(this, [this, historyPath, historyRv, buff, resolve](const ProgressResult& res) {
            if (!res.ret) {
                m_lastCheckResult.ret = res.ret;
                (void)resolve(m_lastCheckResult);
                return;
            }

            // Successfully performed request, update history file...
            UpdateRequestHistory updateRequestHistory = historyRv.val;
            updateRequestHistory.previousRequestDay = QDate::currentDate();
            Ret writeHistoryRet = writeUpdateRequestHistory(historyPath, updateRequestHistory);
            if (!writeHistoryRet) {
                LOGE() << writeHistoryRet.toString();
            }

            QByteArray json = buff->data();
            RetVal<ReleaseInfo> releaseInfo = parseRelease(json);

            if (!releaseInfo.ret) {
                m_lastCheckResult.ret = releaseInfo.ret;
                (void)resolve(m_lastCheckResult);
                return;
            }

            Version current(application()->fullVersion());
            Version update(releaseInfo.val.version);

            bool allowUpdateOnPreRelease = configuration()->allowUpdateOnPreRelease();
            bool isPreRelease = update.preRelease();

            if (!allowUpdateOnPreRelease && isPreRelease) {
                cleanupStalePackages(/*keepFileName*/ std::string());
                m_lastCheckResult.ret = make_ret(Err::NoUpdate);
                (void)resolve(m_lastCheckResult);
                return;
            }

            if (update <= current) {
                cleanupStalePackages(/*keepFileName*/ std::string());
                m_lastCheckResult.ret = make_ret(Err::NoUpdate);
                (void)resolve(m_lastCheckResult);
                return;
            }

            m_lastCheckResult = releaseInfo;

            //! NOTE: Keep an already-downloaded package for this release; drop stale ones.
            cleanupStalePackages(releaseInfo.val.fileName);

            downloadPreviousReleasesNotes(update, [this, resolve](const PrevReleasesNotesList& notes) {
                m_lastCheckResult.val.previousReleasesNotes = notes;
                (void)resolve(m_lastCheckResult);
            });
        });

        return Promise<RetVal<ReleaseInfo> >::dummy_result();
    });
}

const RetVal<ReleaseInfo>& AppUpdateService::lastCheckResult() const
{
    return m_lastCheckResult;
}

RetVal<Progress> AppUpdateService::downloadRelease()
{
    if (!m_networkManager) {
        m_networkManager = networkManagerCreator()->makeNetworkManager();
    }

    const ReleaseInfo info = m_lastCheckResult.val;
    const QUrl fileUrl = QUrl::fromUserInput(QString::fromStdString(info.fileUrl));

    const path_t finalPath = configuration()->updateDataPath() + "/" + info.fileName;
    const path_t partialPath = finalPath + PARTIAL_SUFFIX;
    fileSystem()->makePath(muse::io::absoluteDirpath(partialPath));

    //! NOTE: Resume an interrupted download by appending to the partial file and
    //! requesting the remaining bytes via a Range header.
    uint64_t offset = 0;
    if (fileSystem()->exists(partialPath)) {
        RetVal<uint64_t> sz = fileSystem()->fileSize(partialPath);
        offset = sz.ret ? sz.val : 0;
    }

    RequestHeaders headers;
    io::IODevice::OpenMode mode = io::IODevice::WriteOnly;
    if (offset > 0) {
        headers.rawHeaders["Range"] = QByteArray("bytes=") + QByteArray::number(static_cast<qulonglong>(offset)) + "-";
        mode = io::IODevice::Append;
    }

    auto device = std::make_shared<DownloadFileDevice>(partialPath, mode);

    RetVal<Progress> downloadProgress = m_networkManager->get(fileUrl, device, headers);
    if (!downloadProgress.ret) {
        return RetVal<Progress>::make_ret(downloadProgress.ret);
    }

    m_updateProgress.start();

    m_updateProgress.canceled().onNotify(this, [this, downloadProgress]() {
        Progress mutProgress = downloadProgress.val;
        mutProgress.cancel();
        m_updateProgress.canceled().disconnect(this);
    });

    downloadProgress.val.progressChanged().onReceive(this, [this, offset](int64_t current, int64_t total, const std::string& msg) {
        m_updateProgress.progress(static_cast<int64_t>(offset) + current, static_cast<int64_t>(offset) + total, msg);
    });

    downloadProgress.val.finished().onReceive(this, [this, finalPath, partialPath, offset](const ProgressResult& res) {
        if (!res.ret) {
            //! NOTE: Keep the partial file so the next attempt can resume from it.
            m_updateProgress.finish(ProgressResult::make_ret(res.ret));
            return;
        }

        const int status = res.ret.data<int>("status", 0);

        //! NOTE: We requested a range but the server sent the full file (200) or
        //! rejected the range (416); the partial file is now stale/corrupt - drop
        //! it so the next attempt starts clean.
        if (offset > 0 && (status == 200 || status == 416)) {
            fileSystem()->remove(partialPath);
            m_updateProgress.finish(ProgressResult::make_ret(make_ret(Err::NetworkError, "range request not honoured")));
            return;
        }

        //! Success (200 fresh download or 206 resumed): promote the partial file
        //! to the final package name.
        const Ret ret = fileSystem()->move(partialPath, finalPath, /*replace*/ true);
        if (!ret) {
            m_updateProgress.finish(ProgressResult::make_ret(ret));
            return;
        }

        m_updateProgress.finish(ProgressResult::make_ok(Val(finalPath)));
    });

    return RetVal<Progress>::make_ok(m_updateProgress);
}

RequestHeaders AppUpdateService::prepareHeaders(const UpdateRequestHistory& history) const
{
    RequestHeaders headers = configuration()->updateHeaders();

    if (history.isValid()) {
        std::string iwbString = history.installedWeekBeginning.toString(Qt::ISODate).toStdString();
        QByteArray iwbKey = QByteArray::fromStdString(INSTALLED_WEEK_BEGINNING_KEY.toStdString());
        headers.rawHeaders[iwbKey] = QByteArray::fromStdString(iwbString);

        std::string prdString = history.previousRequestDay.toString(Qt::ISODate).toStdString();
        QByteArray prdKey = QByteArray::fromStdString(PREVIOUS_REQUEST_DAY_KEY.toStdString());
        headers.rawHeaders[prdKey] = QByteArray::fromStdString(prdString);
    }

    return headers;
}

RetVal<AppUpdateService::UpdateRequestHistory> AppUpdateService::readUpdateRequestHistory(const io::path_t& path) const
{
    RetVal<ByteArray> rv = fileSystem()->readFile(path);
    if (!rv.ret) {
        return RetVal<UpdateRequestHistory>(rv.ret);
    }

    QJsonDocument historyDoc = QJsonDocument::fromJson(rv.val.toQByteArrayNoCopy());
    QJsonObject historyObject = historyDoc.object();

    UpdateRequestHistory updateRequestHistory;

    QString installedWeekBeginning = historyObject.value(INSTALLED_WEEK_BEGINNING_KEY).toString();
    updateRequestHistory.installedWeekBeginning = QDate::fromString(installedWeekBeginning, Qt::ISODate);

    QString previousRequestDay = historyObject.value(PREVIOUS_REQUEST_DAY_KEY).toString();
    updateRequestHistory.previousRequestDay = QDate::fromString(previousRequestDay, Qt::ISODate);

    return RetVal<UpdateRequestHistory>::make_ok(updateRequestHistory);
}

Ret AppUpdateService::writeUpdateRequestHistory(const io::path_t& path, const UpdateRequestHistory& updateRequestHistory)
{
    if (!updateRequestHistory.isValid()) {
        return make_ret(Ret::Code::UnknownError);
    }

    QJsonObject historyObject;

    const QDate& iwb = updateRequestHistory.installedWeekBeginning;
    const QDate& prd = updateRequestHistory.previousRequestDay;

    historyObject[INSTALLED_WEEK_BEGINNING_KEY] = iwb.toString(Qt::ISODate);
    historyObject[PREVIOUS_REQUEST_DAY_KEY] = prd.toString(Qt::ISODate);

    QByteArray byteArray = QJsonDocument(historyObject).toJson();

    return fileSystem()->writeFile(path, ByteArray::fromQByteArrayNoCopy(byteArray));
}

RetVal<ReleaseInfo> AppUpdateService::parseRelease(const QByteArray& json) const
{
    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        return make_ret(Err::ReleaseInfoParseError, err.errorString().toStdString());
    }

    QJsonObject release = jsonDoc.object();
    QJsonObject assetObj = resolveReleaseAsset(release);
    if (assetObj.empty()) {
        return make_ret(Err::ReleaseInfoParseError, "No release asset");
    }

    QString tagName = release.value("tag_name").toString();
    QString version = tagName.replace("v", "");
    if (version.isEmpty()) {
        return make_ret(Err::ReleaseInfoParseError, "No release version");
    }

    RetVal<ReleaseInfo> result;
    result.ret = muse::make_ok();
    result.val.fileName = assetObj.value("name").toString().toStdString();
    result.val.fileUrl = assetObj.value("browser_download_url").toString().toStdString();
    result.val.version = version.toStdString();
    result.val.notes = release.value("bodyMarkdown").toString().toStdString();

    return result;
}

std::vector<std::string> AppUpdateService::platformFileSuffixes() const
{
    switch (systemInfo()->productType()) {
    case ISystemInfo::ProductType::Windows: return { "msi" };
    case ISystemInfo::ProductType::MacOS:
        // Prefer the zip bundle for in-place auto-install, falling back to the
        // dmg for the manual flow (and for releases that ship only a dmg).
        if (canAutoInstall()) {
            return { "zip", "dmg" };
        }
        return { "dmg" };
    case ISystemInfo::ProductType::Linux: return { "appimage" };
    case ISystemInfo::ProductType::Unknown: break;
    }

    return {};
}

QJsonObject AppUpdateService::resolveReleaseAsset(const QJsonObject& release) const
{
    const std::vector<std::string> fileSuffixes = platformFileSuffixes();
    ISystemInfo::ProductType productType = systemInfo()->productType();
    ISystemInfo::CpuArchitecture arch = systemInfo()->cpuArchitecture();

    QJsonArray assets = release.value("assets").toArray();
    for (const QJsonValue& asset : release.value("assetsNew").toArray()) {
        assets.push_back(asset);
    }

    // Honour suffix priority: scan all assets for the most preferred suffix
    // before considering the next one.
    for (const std::string& fileSuffix : fileSuffixes) {
        for (const QJsonValue asset : assets) {
            QJsonObject assetObj = asset.toObject();

            QString name = assetObj.value("name").toString();
            if (io::suffix(name) != fileSuffix) {
                continue;
            }

            if (productType == ISystemInfo::ProductType::Linux) {
                if (arch != ISystemInfo::CpuArchitecture::Unknown && arch != assetArch(name)) {
                    continue;
                }
            }

            return assetObj;
        }
    }

    return QJsonObject();
}

bool AppUpdateService::canAutoInstall() const
{
    if (!configuration()->autoInstallEnabled()) {
        return false;
    }

    return updateInstaller()->isInPlaceUpdateSupported();
}

Ret AppUpdateService::applyUpdate(const muse::io::path_t& packagePath)
{
    return updateInstaller()->applyUpdate(packagePath);
}

void AppUpdateService::downloadPreviousReleasesNotes(const Version& updateVersion, const PrevReleaseNotesCallback& finished)
{
    QUrl url = QString::fromStdString(configuration()->previousAppReleasesNotesUrl());
    auto buff = std::make_shared<QBuffer>();
    RetVal<Progress> progress = m_networkManager->get(url, buff);

    if (!progress.ret) {
        LOGE() << "Failed to get previous release notes: " << progress.ret.toString();
        finished({});
        return;
    }

    progress.val.finished().onReceive(this, [this, updateVersion, buff, finished](const ProgressResult& res) {
        PrevReleasesNotesList result;

        DEFER {
            finished(result);
            return;
        };

        if (!res.ret) {
            LOGE() << "Failed to get previous release notes: " << res.ret.toString();
            return;
        }

        QByteArray json = buff->data();
        PrevReleasesNotesList previousReleasesNotes = parsePreviousReleasesNotes(json);
        if (previousReleasesNotes.empty()) {
            return;
        }

        Version currentVersion = Version(application()->fullVersion());

        for (const PrevReleaseNotes& releaseNotes : previousReleasesNotes) {
            Version previousVersion = Version(releaseNotes.version);
            if (updateVersion == previousVersion) {
                continue;
            }

            if (currentVersion < previousVersion) {
                String notesStr = String::fromStdString(releaseNotes.notes);
                if (notesStr.startsWith(u"###")) {
                    //! Release notes may be in the format of: ### MuseScore x.y.z is now available!\r\n...notes...\r\n
                    //! We need to remove the title of the release notes to get the actual notes.
                    static const std::regex titleRegex(R"(^###.*?\r\n)");
                    std::string notesWithoutTitle = notesStr.remove(titleRegex).toStdString();
                    result.emplace_back(releaseNotes.version, notesWithoutTitle);
                } else {
                    result.emplace_back(releaseNotes.version, releaseNotes.notes);
                }
            }
        }

        std::sort(result.begin(), result.end(), [](const PrevReleaseNotes& a,
                                                   const PrevReleaseNotes& b) {
            return Version(a.version) < Version(b.version);
        });
    });
}

void AppUpdateService::clear()
{
    m_lastCheckResult = RetVal<ReleaseInfo>::make_ok(ReleaseInfo());
}

void AppUpdateService::cleanupStalePackages(const std::string& keepFileName)
{
#if !defined(Q_OS_LINUX)
    const io::path_t dir = configuration()->updateDataPath();
    if (!fileSystem()->exists(dir)) {
        return;
    }

    //! NOTE: No relevant package to keep -> drop everything.
    if (keepFileName.empty()) {
        fileSystem()->remove(dir);
        return;
    }

    RetVal<io::paths_t> entries = fileSystem()->scanFiles(dir, {}, io::ScanMode::FilesAndFoldersInCurrentDir);
    if (!entries.ret) {
        return;
    }

    //! NOTE: Keep both the finished package and its in-progress ".part" file so an
    //! interrupted download of the current release can still be resumed.
    const std::string keepPartial = keepFileName + PARTIAL_SUFFIX;

    for (const io::path_t& entry : entries.val) {
        const std::string name = io::filename(entry).toStdString();
        if (name != keepFileName && name != keepPartial) {
            fileSystem()->remove(entry);
        }
    }
#else
    UNUSED(keepFileName);
#endif
}

bool AppUpdateService::isReleaseDownloaded() const
{
    return !downloadedReleasePath().empty();
}

io::path_t AppUpdateService::downloadedReleasePath() const
{
    if (!m_lastCheckResult.ret) {
        return {};
    }

    const std::string& fileName = m_lastCheckResult.val.fileName;
    if (fileName.empty()) {
        return {};
    }

    const io::path_t path = configuration()->updateDataPath() + "/" + fileName;
    if (!fileSystem()->exists(path)) {
        return {};
    }

    return path;
}
