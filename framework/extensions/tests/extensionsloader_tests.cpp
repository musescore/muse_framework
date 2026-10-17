/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) MuseScore/Audacity and others
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

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "extensions/extensionbundle.h"
#include "extensions/internal/extensionsloader.h"
#include "extensions/internal/legacy/extpluginsloader.h"

using namespace muse;
using namespace muse::extensions;

//! Dock forms retain their presentation without changing ordinary dialogs or macros.
TEST(ExtensionsLoaderTests, LegacyDockPresentation)
{
    QTemporaryDir directory;
    QTemporaryDir emptyDirectory;
    ASSERT_TRUE(directory.isValid());
    for (const QString& type : { QString("dock"), QString("dialog"), QString(""), QString("unknown") }) {
        QFile file(directory.filePath("plugin.qml"));
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        const QByteArray qml = QString("import MuseScore 3.0\nMuseScore {\n"
                                       "title: \"Test\"\npluginType: \"%1\"\nrequiresScore: false\n}\n").arg(type).toUtf8();
        ASSERT_EQ(file.write(qml), qml.size());
        file.close();
        const auto manifests = legacy::ExtPluginsLoader().loadManifestList(
            io::path_t(directory.path()), io::path_t(emptyDirectory.path()));
        ASSERT_EQ(manifests.size(), 1u);
        ASSERT_EQ(manifests.front().actions.size(), 1u);
        const Action& action = manifests.front().actions.front();
        EXPECT_EQ(action.docked, type == "dock");
        EXPECT_EQ(action.context, type == "dock" ? PROJECT_OPENED_CONTEXT : ANY_CONTEXT);
        EXPECT_EQ(action.type, type == "dock" || type == "dialog" ? Type::Form : Type::Macros);
    }
}

TEST(ExtensionBundleTests, ResolvesFileFromBundleDirectory) {
    QTemporaryDir bundle;
    ASSERT_TRUE(bundle.isValid());
    ASSERT_TRUE(QDir().mkpath(bundle.filePath(QStringLiteral("scripts"))));

    QFile script(bundle.filePath(QStringLiteral("scripts/effect.js")));
    ASSERT_TRUE(script.open(QIODevice::WriteOnly));
    script.close();

    const auto resolved = resolveBundleFile(io::path_t(bundle.path()), io::path_t("scripts/effect.js"));
    ASSERT_TRUE(resolved);
    EXPECT_EQ(resolved->toQString(), QFileInfo(script).canonicalFilePath());
    EXPECT_FALSE(resolveBundleFile(io::path_t(bundle.path()), io::path_t("../effect.js")));
}

TEST(ExtensionsLoaderTests, ParsesContributions) {
    constexpr const char* json
        =
            R"({
        "uri": "audacity://extensions/test",
        "type": "macros",
        "title": "Test",
        "actions": [],
        "contributes": {
            "audacity.effects": [{
                "id": "gain",
                "enabled": true,
                "scale": 0.5,
                "choices": ["low", "high"],
                "metadata": { "vendor": "Audacity" }
            }],
            "ignored": "not an array"
        }
    })";

    const auto result = ExtensionsLoader().parseManifest(ByteArray(json));

    ASSERT_TRUE(result.ret);
    ASSERT_EQ(result.val.contributes.size(), 1u);
    const auto effects = result.val.contributes.find("audacity.effects");
    ASSERT_NE(effects, result.val.contributes.end());
    ASSERT_EQ(effects->second.size(), 1u);

    const ValMap& effect = effects->second.front();
    EXPECT_EQ(effect.at("id").toString(), "gain");
    EXPECT_TRUE(effect.at("enabled").toBool());
    EXPECT_DOUBLE_EQ(effect.at("scale").toDouble(), 0.5);

    const ValList choices = effect.at("choices").toList();
    ASSERT_EQ(choices.size(), 2u);
    EXPECT_EQ(choices[0].toString(), "low");
    EXPECT_EQ(choices[1].toString(), "high");
    EXPECT_EQ(effect.at("metadata").toMap().at("vendor").toString(), "Audacity");
}
