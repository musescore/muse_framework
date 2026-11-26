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

#ifndef NO_QT_SUPPORT

#include <gtest/gtest.h>

#include <memory>

#include <QDir>
#include <QImage>
#include <QMimeData>

#include "global/qt/qtmimedataconvert.h"

using namespace muse;
using namespace muse::draw;

namespace {
io::path_t makeFilePath()
{
    return io::path_t(QDir::cleanPath(QDir::tempPath() + "/muse_mime_data_test.txt"));
}

std::shared_ptr<Pixmap> makeImage()
{
    QImage image(4, 3, QImage::Format_ARGB32);
    image.fill(Qt::red);
    return std::make_shared<Pixmap>(Pixmap::fromQImage(image));
}
}

TEST(Global_QtMimeDataTests, ToQtMimeData_CopiesFormatsAndText)
{
    //! [GIVEN] Mime data with a format and a text
    MimeData data;
    data.setData("application/test", ByteArray("abc"));
    data.setText(u"hello");

    //! [WHEN] Converting it to Qt
    std::unique_ptr<QMimeData> qdata(toQtMimeData(data));

    //! [THEN] Qt has the format and the text
    EXPECT_TRUE(qdata->hasFormat("application/test"));
    EXPECT_EQ(qdata->data("application/test"), QByteArray("abc"));
    EXPECT_EQ(qdata->text(), QString("hello"));
}

TEST(Global_QtMimeDataTests, ToQtMimeData_Image_IsStoredAsQImage)
{
    //! [GIVEN] Mime data with an image
    MimeData data;
    data.setImageData(makeImage());

    //! [WHEN] Converting it to Qt
    std::unique_ptr<QMimeData> qdata(toQtMimeData(data));

    //! [THEN] The image is a QImage, that the standard receivers of Qt can read
    ASSERT_TRUE(qdata->hasImage());
    const QVariant variant = qdata->imageData();
    EXPECT_EQ(variant.typeId(), QMetaType::QImage);

    const QImage image = qvariant_cast<QImage>(variant);
    EXPECT_EQ(image.width(), 4);
    EXPECT_EQ(image.height(), 3);
}

TEST(Global_QtMimeDataTests, ToQtMimeData_LocalFiles_AreStoredAsUrls)
{
    //! [GIVEN] Mime data with a local file
    MimeData data;
    data.setLocalFiles({ makeFilePath() });

    //! [WHEN] Converting it to Qt
    std::unique_ptr<QMimeData> qdata(toQtMimeData(data));

    //! [THEN] Qt has the url of the local file
    ASSERT_TRUE(qdata->hasUrls());
    ASSERT_EQ(qdata->urls().size(), 1);
    EXPECT_TRUE(qdata->urls().first().isLocalFile());
    EXPECT_EQ(qdata->urls().first().toLocalFile(), makeFilePath().toQString());
}

TEST(Global_QtMimeDataTests, FromQtMimeData_AfterToQtMimeData_KeepsTheContent)
{
    //! [GIVEN] Mime data with a format, a text, an image and a local file, converted to Qt
    MimeData data;
    data.setData("application/test", ByteArray("abc"));
    data.setText(u"hello");
    data.setImageData(makeImage());
    data.setLocalFiles({ makeFilePath() });
    std::unique_ptr<QMimeData> qdata(toQtMimeData(data));

    //! [WHEN] Converting it back
    const MimeData result = fromQtMimeData(qdata.get());

    //! [THEN] The content is the same
    EXPECT_EQ(result.data("application/test"), ByteArray("abc"));
    EXPECT_EQ(result.text(), String(u"hello"));
    ASSERT_TRUE(result.hasImage());
    EXPECT_EQ(result.imageData()->width(), 4);
    EXPECT_EQ(result.imageData()->height(), 3);
    ASSERT_EQ(result.localFiles().size(), 1u);
    EXPECT_EQ(result.localFiles().front(), makeFilePath());
}

TEST(Global_QtMimeDataTests, FromQtMimeData_GivenFormats_CopiesOnlyThem)
{
    //! [GIVEN] Qt mime data with two formats
    QMimeData qdata;
    qdata.setData("application/first", QByteArray("1"));
    qdata.setData("application/second", QByteArray("2"));

    //! [WHEN] Converting only the first
    const MimeData result = fromQtMimeData(&qdata, { "application/first" });

    //! [THEN] The second is not copied
    EXPECT_TRUE(result.hasFormat("application/first"));
    EXPECT_FALSE(result.hasFormat("application/second"));
}

TEST(Global_QtMimeDataTests, FromQtMimeData_NullData_ProvidesEmptyMimeData)
{
    EXPECT_TRUE(fromQtMimeData(nullptr).isEmpty());
}

#endif // NO_QT_SUPPORT
