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
#include <gtest/gtest.h>

#include <QLocale>

#include "ui/view/qmldataformatter.h"

using namespace muse::ui;

class Ui_QmlDataFormatterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_previousLocale = QLocale();
    }

    void TearDown() override
    {
        QLocale::setDefault(m_previousLocale);
    }

    QmlDataFormatter m_formatter;
    QLocale m_previousLocale;
};

TEST_F(Ui_QmlDataFormatterTests, FormatReal_KeepsGroupSeparator)
{
    QLocale::setDefault(QLocale("en_US"));
    EXPECT_EQ(m_formatter.formatReal(1234.5, 2), "1,234.5");

    QLocale::setDefault(QLocale("de_DE"));
    EXPECT_EQ(m_formatter.formatReal(1234.5, 2), "1.234,5");
}

TEST_F(Ui_QmlDataFormatterTests, FormatRealForEdit_NoGroupSeparator)
{
    QLocale::setDefault(QLocale("en_US"));
    EXPECT_EQ(m_formatter.formatRealForEdit(1234.5, 2), "1234.5");
    EXPECT_EQ(m_formatter.formatRealForEdit(1234.0, 2), "1234");
    EXPECT_EQ(m_formatter.formatRealForEdit(1234.0, 0), "1234");

    QLocale::setDefault(QLocale("de_DE"));
    EXPECT_EQ(m_formatter.formatRealForEdit(1234.5, 2), "1234,5");

    QLocale::setDefault(QLocale("fr_FR"));
    EXPECT_EQ(m_formatter.formatRealForEdit(1234.5, 2), "1234,5");
}

TEST_F(Ui_QmlDataFormatterTests, ParseReal_PlainAndLocalizedInput)
{
    QLocale::setDefault(QLocale("en_US"));
    EXPECT_EQ(m_formatter.parseReal("1234.5").toDouble(), 1234.5);
    EXPECT_EQ(m_formatter.parseReal("1,234.5").toDouble(), 1234.5);
    EXPECT_EQ(m_formatter.parseReal("-42").toDouble(), -42.0);

    QLocale::setDefault(QLocale("de_DE"));
    EXPECT_EQ(m_formatter.parseReal("1234,5").toDouble(), 1234.5);
    EXPECT_EQ(m_formatter.parseReal("1.234,5").toDouble(), 1234.5);
}

TEST_F(Ui_QmlDataFormatterTests, ParseReal_SpaceGroupedLocaleAcceptsPastedSpaces)
{
    QLocale::setDefault(QLocale("fr_FR"));
    EXPECT_EQ(m_formatter.parseReal("1 234,5").toDouble(), 1234.5); // plain space
    EXPECT_EQ(m_formatter.parseReal(QString("1\u00A0234,5")).toDouble(), 1234.5); // no-break space
    EXPECT_EQ(m_formatter.parseReal(QString("1\u202F234,5")).toDouble(), 1234.5); // narrow no-break space
}

TEST_F(Ui_QmlDataFormatterTests, ParseReal_PartialInputIsInvalid)
{
    QLocale::setDefault(QLocale("en_US"));
    EXPECT_FALSE(m_formatter.parseReal("").isValid());
    EXPECT_FALSE(m_formatter.parseReal("-").isValid());
    EXPECT_FALSE(m_formatter.parseReal("+").isValid());
    EXPECT_FALSE(m_formatter.parseReal(".").isValid());
    EXPECT_FALSE(m_formatter.parseReal("-.").isValid());
    EXPECT_FALSE(m_formatter.parseReal("abc").isValid());
}

TEST_F(Ui_QmlDataFormatterTests, ParseReal_TrailingDecimalSeparatorIsChopped)
{
    QLocale::setDefault(QLocale("en_US"));
    EXPECT_EQ(m_formatter.parseReal("1.").toDouble(), 1.0);

    QLocale::setDefault(QLocale("de_DE"));
    EXPECT_EQ(m_formatter.parseReal("1,").toDouble(), 1.0);
}

TEST_F(Ui_QmlDataFormatterTests, ParseReal_RoundsToDecimals)
{
    QLocale::setDefault(QLocale("en_US"));
    EXPECT_EQ(m_formatter.parseReal("1.006", 2).toDouble(), 1.01);
    EXPECT_EQ(m_formatter.parseReal("1.004", 2).toDouble(), 1.0);
    EXPECT_EQ(m_formatter.parseReal("1.006").toDouble(), 1.006); // no rounding by default
}

TEST_F(Ui_QmlDataFormatterTests, RoundReal_RemovesFloatingPointDust)
{
    EXPECT_EQ(m_formatter.roundReal(0.1 + 0.2, 2), 0.3);
    EXPECT_EQ(m_formatter.roundReal(1.005 * 100, 0), 100.0);
}
