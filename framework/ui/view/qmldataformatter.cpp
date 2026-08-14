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

#include "global/dataformatter.h"

using namespace muse::ui;

QmlDataFormatter::QmlDataFormatter(QObject* parent)
    : QObject{parent}
{
}

QString QmlDataFormatter::formatReal(double value, int decimals) const
{
    return formatWithLocale(QLocale(), value, decimals);
}

QString QmlDataFormatter::formatRealForEdit(double value, int decimals) const
{
    QLocale locale;
    locale.setNumberOptions(locale.numberOptions() | QLocale::OmitGroupSeparator);
    return formatWithLocale(locale, value, decimals);
}

QVariant QmlDataFormatter::parseReal(const QString& text, int decimals) const
{
    const QLocale locale;
    QString t = text.trimmed();

    // Group separators are display-only; drop them. When the locale groups
    // with a space variant, also drop plain/no-break spaces so pasted text parses.
    const QString groupSep = locale.groupSeparator();
    t.remove(groupSep);
    if (!groupSep.isEmpty() && groupSep.at(0).isSpace()) {
        t.remove(QChar(' '));
        t.remove(QChar(0x00A0)); // no-break space
        t.remove(QChar(0x202F)); // narrow no-break space
    }

    const QString decSep = locale.decimalPoint();
    if (t.endsWith(decSep)) {
        t.chop(decSep.length());
    }

    if (t.isEmpty() || t == "-" || t == "+") {
        return QVariant();
    }

    bool ok = false;
    double value = locale.toDouble(t, &ok);
    if (!ok) {
        return QVariant();
    }

    return decimals >= 0 ? DataFormatter::roundDouble(value, decimals) : value;
}

double QmlDataFormatter::roundReal(double value, int decimals) const
{
    return DataFormatter::roundDouble(value, decimals);
}

QString QmlDataFormatter::formatWithLocale(const QLocale& locale, double value, int decimals)
{
    QString formatted = locale.toString(value, 'f', decimals);
    if (decimals > 0) {
        // Remove trailing zeros after the decimal separator
        QString decSepStr = locale.decimalPoint();
        QChar decSep = decSepStr.isEmpty() ? QChar('.') : decSepStr.at(0);
        int decPos = formatted.indexOf(decSep);
        if (decPos != -1) {
            int last = formatted.length() - 1;
            // Remove trailing zeros
            while (last > decPos && formatted[last] == '0') {
                --last;
            }
            // Remove trailing decimal separator if needed
            if (last == decPos) {
                --last;
            }
            formatted = formatted.left(last + 1);
        }
    }
    return formatted;
}
