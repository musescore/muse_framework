/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore/Audacity and others
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
#include <qvalidator.h>

#include "../validators/intinputvalidator.h"

// Teach GoogleTest how to print QString so failure diffs are readable
// instead of a UTF-16 byte dump.
inline void PrintTo(const QString& s, std::ostream* os)
{
    *os << '"' << s.toStdString() << '"';
}

using namespace muse;
using namespace muse::uicomponents;

namespace muse::uicomponents {
struct Input
{
    QString str;
    QValidator::State expectedState;
    QString fixedStr = {};
};

class IntInputValidatorTests : public ::testing::Test, public QObject
{
public:
    void SetUp() override
    {
        m_validator = new IntInputValidator(nullptr);
    }

    void TearDown() override
    {
        delete m_validator;
    }

protected:
    IntInputValidator* m_validator = nullptr;

    void runInputTests(const std::vector<Input>& inputs)
    {
        int pos = 0;
        for (const Input& input : inputs) {
            QString inputCopy = input.str;
            auto state = m_validator->validate(inputCopy, pos);
            EXPECT_EQ(state, input.expectedState) << "validate(\"" << input.str.toStdString() << "\")";

            if (QValidator::Invalid == input.expectedState) {
                continue;
            }

            QString fixInput = input.str;
            m_validator->fixup(fixInput);

            QString expectedStr = input.fixedStr.isEmpty() ? input.str : input.fixedStr;
            EXPECT_EQ(expectedStr, fixInput) << "fixup(\"" << input.str.toStdString() << "\")";
        }
    }
};

TEST_F(IntInputValidatorTests, ValidateCommaLocale) {
    QLocale prev = QLocale();
    QLocale::setDefault(QLocale("en_US"));

    m_validator->setTop(48000);
    m_validator->setBottom(-48000);

    runInputTests({
            { "0", QValidator::Acceptable },
            { "1", QValidator::Acceptable },
            { "100", QValidator::Acceptable },
            { "1,000", QValidator::Acceptable },
            { "1000", QValidator::Acceptable, "1,000" },
            { "1,0000", QValidator::Acceptable, "10,000" },
            { "48,000", QValidator::Acceptable },
            { "48,001", QValidator::Intermediate, "48,000" },
            { "-100", QValidator::Acceptable },
            { "-1,000", QValidator::Acceptable },
            { "-1000", QValidator::Acceptable, "-1,000" },
            { "-1,0000", QValidator::Acceptable, "-10,000" },
            { "-48,000", QValidator::Acceptable },
            { "-48,001", QValidator::Intermediate, "-48,000" },
            { "2147483647", QValidator::Invalid },
            { "-2147483648", QValidator::Invalid },
            { "abc", QValidator::Invalid },
            { "", QValidator::Intermediate, "0" }
        });

    m_validator->setTop(10);
    m_validator->setBottom(1);

    runInputTests({
            { "0", QValidator::Intermediate, "1" },
            { "", QValidator::Intermediate, "1" },
            { "1", QValidator::Acceptable }
        });

    QLocale::setDefault(prev);
}

TEST_F(IntInputValidatorTests, PartialInputStaysTypeable) {
    QLocale prev = QLocale();
    QLocale::setDefault(QLocale("en_US"));

    // A minimum above 9 must not reject every single-digit prefix: "4" has to
    // survive so it can become "40"
    m_validator->setTop(240);
    m_validator->setBottom(10);

    runInputTests({
            { "4", QValidator::Intermediate, "10" },
            { "40", QValidator::Acceptable },
            { "240", QValidator::Acceptable },
            { "241", QValidator::Intermediate, "240" },
            { "", QValidator::Intermediate, "10" }
        });

    // A minimum above 0 must not make the empty field invalid, or the text
    // can never be cleared and retyped
    m_validator->setTop(30);
    m_validator->setBottom(1);

    runInputTests({
            { "", QValidator::Intermediate, "1" },
            { "0", QValidator::Intermediate, "1" },
            { "5", QValidator::Acceptable },
            { "58", QValidator::Intermediate, "30" }
        });

    QLocale::setDefault(prev);
}

TEST_F(IntInputValidatorTests, ValidateDotLocale) {
    QLocale prev = QLocale();
    QLocale::setDefault(QLocale("ro_RO"));

    m_validator->setTop(48000);
    m_validator->setBottom(-48000);

    runInputTests({
            { "0", QValidator::Acceptable },
            { "1", QValidator::Acceptable },
            { "100", QValidator::Acceptable },
            { "1.000", QValidator::Acceptable },
            { "1000", QValidator::Acceptable, "1.000" },
            { "1.0000", QValidator::Acceptable, "10.000" },
            { "48.000", QValidator::Acceptable },
            { "48.001", QValidator::Intermediate, "48.000" },
            { "-100", QValidator::Acceptable },
            { "-1.000", QValidator::Acceptable },
            { "-1000", QValidator::Acceptable, "-1.000" },
            { "-1.0000", QValidator::Acceptable, "-10.000" },
            { "-48.000", QValidator::Acceptable },
            { "-48.001", QValidator::Intermediate, "-48.000" },
            { "abc", QValidator::Invalid },
            { "", QValidator::Intermediate, "0" }
        });

    QLocale::setDefault(prev);
}
}
