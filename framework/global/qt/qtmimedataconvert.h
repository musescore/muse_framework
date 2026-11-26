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

#ifndef NO_QT_SUPPORT

#include <string>
#include <vector>

#include <QImage>
#include <QMimeData>
#include <QUrl>
#include <QVariant>

#include "types/mimedata.h"

//! NOTE Conversion of QMimeData to MimeData and back
namespace muse {
//! NOTE If the formats are given, only they and the local files are copied (it is for the frequent drag events),
//! otherwise everything is copied, including the image
inline MimeData fromQtMimeData(const QMimeData* qdata, const std::vector<std::string>& formats = {})
{
    MimeData result;
    if (!qdata) {
        return result;
    }

    if (formats.empty()) {
        for (const QString& format : qdata->formats()) {
            result.setData(format.toStdString(), ByteArray::fromQByteArray(qdata->data(format)));
        }
    } else {
        for (const std::string& format : formats) {
            const QString qformat = QString::fromStdString(format);
            if (qdata->hasFormat(qformat)) {
                result.setData(format, ByteArray::fromQByteArray(qdata->data(qformat)));
            }
        }
    }

    if (formats.empty() && qdata->hasImage()) {
        const QVariant image = qdata->imageData();
        if (image.canConvert<draw::Pixmap>()) {
            result.setImageData(std::make_shared<draw::Pixmap>(image.value<draw::Pixmap>()));
        } else if (image.canConvert<QImage>()) {
            result.setImageData(std::make_shared<draw::Pixmap>(draw::Pixmap::fromQImage(image.value<QImage>())));
        }
    }

    std::vector<io::path_t> localFiles;
    for (const QUrl& url : qdata->urls()) {
        if (url.isLocalFile()) {
            localFiles.push_back(io::path_t(url.toLocalFile()));
        }
    }
    result.setLocalFiles(localFiles);

    return result;
}

//! NOTE The caller takes the ownership of the result
inline QMimeData* toQtMimeData(const MimeData& data)
{
    QMimeData* result = new QMimeData();
    for (const std::string& format : data.formats()) {
        result->setData(QString::fromStdString(format), data.data(format).toQByteArray());
    }

    if (data.hasImage()) {
        //! NOTE QImage, because it is the standard type of the image data, that the receivers of Qt can read
        result->setImageData(draw::Pixmap::toQImage(*data.imageData()));
    }

    if (!data.localFiles().empty()) {
        QList<QUrl> urls;
        for (const io::path_t& path : data.localFiles()) {
            urls.push_back(QUrl::fromLocalFile(path.toQString()));
        }
        result->setUrls(urls);
    }

    return result;
}
}

#endif // NO_QT_SUPPORT
