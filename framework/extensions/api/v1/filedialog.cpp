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
#include "filedialog.h"

#include "log.h"

using namespace muse;
using namespace muse::extensions::apiv1;

FileDialog::FileDialog(QObject* parent)
    : QObject(parent), Contextable(muse::iocCtxForQmlObject(this)) {}

void FileDialog::doOpen()
{
    async::Promise<io::path_t> promise = m_type == Load
                                         ? interactive()->selectOpeningFile(m_title.toStdString(), m_folder, {})
                                         : interactive()->selectSavingFile(m_title.toStdString(), m_folder, {});

    const int request = m_request;

    promise.onResolve(this, [this, request](const io::path_t& path) {
        LOGD() << "selected file: " << path;
        onClosed(request, path.toQString());
    }).onReject(this, [this, request](int, const std::string&) {
        onClosed(request, QString());
    });
}

void FileDialog::onClosed(int request, const QString& filePath)
{
    //! NOTE The dialog was closed or opened again meanwhile
    if (request != m_request) {
        return;
    }

    setFilePath(filePath);
    setVisible(false);

    if (!m_filePath.isEmpty()) {
        emit accepted();
    } else {
        emit rejected();
    }
}

QString FileDialog::title() const
{
    return m_title;
}

void FileDialog::setTitle(const QString& newTitle)
{
    if (m_title == newTitle) {
        return;
    }
    m_title = newTitle;
    emit titleChanged();
}

bool FileDialog::visible() const
{
    return m_visible;
}

void FileDialog::setVisible(bool newVisible)
{
    if (m_visible == newVisible) {
        return;
    }
    m_visible = newVisible;
    emit visibleChanged();

    if (m_visible) {
        doOpen();
    } else {
        ++m_request;
    }
}

void FileDialog::open()
{
    setVisible(true);
}

void FileDialog::close()
{
    setVisible(false);
}

QString FileDialog::folder() const
{
    return m_folder;
}

void FileDialog::setFolder(const QString& newFolder)
{
    if (m_folder == newFolder) {
        return;
    }
    m_folder = newFolder;
    emit folderChanged();
}

QString FileDialog::filePath() const
{
    return m_filePath;
}

void FileDialog::setFilePath(const QString& newFilePath)
{
    if (m_filePath == newFilePath) {
        return;
    }
    m_filePath = newFilePath;
    emit filePathChanged();
}

QUrl FileDialog::fileUrl() const
{
    return QUrl::fromLocalFile(m_filePath);
}

void FileDialog::setFileUrl(const QUrl& newFileUrl)
{
    setFilePath(newFileUrl.toLocalFile());
}

QList<QUrl> FileDialog::fileUrls() const
{
    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(m_filePath));
    return urls;
}

void FileDialog::setFileUrls(const QList<QUrl>& newFileUrls)
{
    if (newFileUrls.isEmpty()) {
        setFilePath(QString());
    } else {
        setFilePath(newFileUrls.first().toLocalFile());
    }
}

FileDialog::Type FileDialog::type() const
{
    return m_type;
}

void FileDialog::setType(Type newType)
{
    if (m_type == newType) {
        return;
    }
    m_type = newType;
    emit typeChanged();
}
