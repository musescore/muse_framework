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
#pragma once

#include <QLocale>
#include <QObject>
#include <QVariant>
#include <qqmlintegration.h>

namespace muse::ui {
class QmlDataFormatter : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(DataFormatter);
    QML_UNCREATABLE("Must be created in C++ only")

public:
    explicit QmlDataFormatter(QObject* parent = nullptr);

    //! Locale-aware display formatting (keeps the locale's group separators)
    Q_INVOKABLE QString formatReal(double value, int decimals = 2) const;

    //! Like formatReal, but never emits group separators; use for editable fields
    Q_INVOKABLE QString formatRealForEdit(double value, int decimals = 2) const;

    //! Locale-aware parse of user input. Returns the number, or an invalid
    //! QVariant (undefined in QML) for empty/partial/unparseable text.
    //! decimals >= 0 rounds the result.
    Q_INVOKABLE QVariant parseReal(const QString& text, int decimals = -1) const;

    Q_INVOKABLE double roundReal(double value, int decimals = 2) const;

private:
    static QString formatWithLocale(const QLocale& locale, double value, int decimals);
};
}
