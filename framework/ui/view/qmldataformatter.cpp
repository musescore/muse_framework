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
#include "qmldataformatter.h"

#include <QLocale>

#include "global/dataformatter.h"

using namespace muse::ui;

QmlDataFormatter::QmlDataFormatter(QObject* parent)
    : QObject{parent}
{
}

QString QmlDataFormatter::formatReal(double value, int decimals) const
{
    return DataFormatter::formatLocalizedReal(value, decimals).toQString();
}

QString QmlDataFormatter::formatRealForEdit(double value, int decimals) const
{
    return DataFormatter::formatLocalizedReal(value, decimals, true).toQString();
}

QVariant QmlDataFormatter::parseReal(const QString& text, int decimals) const
{
    const QLocale locale;
    QString str = text.trimmed();

    // Group separators are display-only; drop them, including the space
    // variants that space-grouping locales may paste in
    const QString groupSep = locale.groupSeparator();
    str.remove(groupSep);
    if (!groupSep.isEmpty() && groupSep.at(0).isSpace()) {
        str.remove(QChar(' '));
        str.remove(QChar(0x00A0));
        str.remove(QChar(0x202F));
    }

    const QString decSep = locale.decimalPoint();
    if (str.endsWith(decSep)) {
        str.chop(decSep.size());
    }

    if (str.isEmpty() || str == "-" || str == "+") {
        return QVariant();
    }

    bool ok = false;
    const double value = locale.toDouble(str, &ok);
    if (!ok) {
        return QVariant();
    }

    return decimals >= 0 ? DataFormatter::roundDouble(value, decimals) : value;
}

double QmlDataFormatter::roundReal(double value, int decimals) const
{
    return DataFormatter::roundDouble(value, decimals);
}

int QmlDataFormatter::decimalsForStep(double step) const
{
    return DataFormatter::decimalsForStep(step);
}
