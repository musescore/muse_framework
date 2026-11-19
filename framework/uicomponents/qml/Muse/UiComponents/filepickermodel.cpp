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

#include "filepickermodel.h"

using namespace muse::uicomponents;

FilePickerModel::FilePickerModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

QString FilePickerModel::title() const
{
    return m_title;
}

QString FilePickerModel::dir() const
{
    return m_dir;
}

QStringList FilePickerModel::filter() const
{
    return m_filter;
}

std::vector<std::string> FilePickerModel::filterList() const
{
    std::vector<std::string> filter;
    for (const QString& nameFilter : m_filter) {
        filter.push_back(nameFilter.toStdString());
    }

    return filter;
}

void FilePickerModel::onFileSelected(const io::path_t& file)
{
    if (file.empty()) {
        return;
    }

    m_dir = io::dirpath(file).toQString();
    emit pathSelected(file.toQString());
}

void FilePickerModel::selectFile()
{
    interactive()->selectOpeningFile(m_title.toStdString(), m_dir, filterList())
    .onResolve(this, [this](const io::path_t& file) {
        onFileSelected(file);
    });
}

void FilePickerModel::selectDirectory()
{
    interactive()->selectDirectory(m_title.toStdString(), m_dir)
    .onResolve(this, [this](const io::path_t& directory) {
        if (directory.empty()) {
            return;
        }

        m_dir = directory.toQString();
        emit pathSelected(directory.toQString());
    });
}

void FilePickerModel::selectMultipleDirectories(const QString& selectedDirectoriesStr)
{
    io::paths_t selectedDirectories = io::pathsFromString(selectedDirectoriesStr.toStdString());
    interactive()->selectMultipleDirectories(m_title.toStdString(), m_dir, selectedDirectories)
    .onResolve(this, [this](const io::paths_t& directories) {
        QStringList result;
        for (const io::path_t& dir: directories) {
            result << dir.toQString();
        }

        emit pathSelected(result.join(";"));
    });
}

void FilePickerModel::selectAny()
{
    interactive()->selectSavingFile(m_title.toStdString(), m_dir, filterList())
    .onResolve(this, [this](const io::path_t& file) {
        onFileSelected(file);
    });
}

void FilePickerModel::setTitle(const QString& title)
{
    if (title == m_title) {
        return;
    }

    m_title = title;
    emit titleChanged(title);
}

void FilePickerModel::setDir(const QString& dir)
{
    if (dir == m_dir) {
        return;
    }

    m_dir = dir;
    emit dirChanged(dir);
}

void FilePickerModel::setFilter(const QStringList& filter)
{
    if (filter == m_filter) {
        return;
    }

    m_filter = filter;
    emit filterChanged(filter);
}
