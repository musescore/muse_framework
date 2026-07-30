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

#include "polylinepointstyle.h"

using namespace muse::uicomponents;

PolylinePointStyle::PolylinePointStyle(QObject* parent)
    : QObject(parent)
{
}

qreal PolylinePointStyle::centerRadius() const
{
    return m_centerRadius;
}

void PolylinePointStyle::setCenterRadius(qreal r)
{
    if (m_centerRadius == r) {
        return;
    }

    m_centerRadius = r;
    emit styleChanged();
}

qreal PolylinePointStyle::centerRadiusHovered() const
{
    return m_centerRadiusHovered;
}

void PolylinePointStyle::setCenterRadiusHovered(qreal r)
{
    if (m_centerRadiusHovered == r) {
        return;
    }

    m_centerRadiusHovered = r;
    emit styleChanged();
}

QColor PolylinePointStyle::centerColor() const
{
    return m_centerColor;
}

void PolylinePointStyle::setCenterColor(const QColor& c)
{
    if (m_centerColor == c) {
        return;
    }

    m_centerColor = c;
    emit styleChanged();
}

QColor PolylinePointStyle::centerColorHovered() const
{
    return m_centerColorHovered;
}

void PolylinePointStyle::setCenterColorHovered(const QColor& c)
{
    if (m_centerColorHovered == c) {
        return;
    }

    m_centerColorHovered = c;
    emit styleChanged();
}

qreal PolylinePointStyle::middleRingWidth() const
{
    return m_middleRingWidth;
}

void PolylinePointStyle::setMiddleRingWidth(qreal w)
{
    if (m_middleRingWidth == w) {
        return;
    }

    m_middleRingWidth = w;
    emit styleChanged();
}

qreal PolylinePointStyle::middleRingWidthHovered() const
{
    return m_middleRingWidthHovered;
}

void PolylinePointStyle::setMiddleRingWidthHovered(qreal w)
{
    if (m_middleRingWidthHovered == w) {
        return;
    }

    m_middleRingWidthHovered = w;
    emit styleChanged();
}

QColor PolylinePointStyle::middleRingColor() const
{
    return m_middleRingColor;
}

void PolylinePointStyle::setMiddleRingColor(const QColor& c)
{
    if (m_middleRingColor == c) {
        return;
    }

    m_middleRingColor = c;
    emit styleChanged();
}

QColor PolylinePointStyle::middleRingColorHovered() const
{
    return m_middleRingColorHovered;
}

void PolylinePointStyle::setMiddleRingColorHovered(const QColor& c)
{
    if (m_middleRingColorHovered == c) {
        return;
    }

    m_middleRingColorHovered = c;
    emit styleChanged();
}

qreal PolylinePointStyle::outlineWidth() const
{
    return m_outlineWidth;
}

void PolylinePointStyle::setOutlineWidth(qreal w)
{
    if (m_outlineWidth == w) {
        return;
    }

    m_outlineWidth = w;
    emit styleChanged();
}

qreal PolylinePointStyle::outlineWidthHovered() const
{
    return m_outlineWidthHovered;
}

void PolylinePointStyle::setOutlineWidthHovered(qreal w)
{
    if (m_outlineWidthHovered == w) {
        return;
    }

    m_outlineWidthHovered = w;
    emit styleChanged();
}

QColor PolylinePointStyle::outlineColor() const
{
    return m_outlineColor;
}

void PolylinePointStyle::setOutlineColor(const QColor& c)
{
    if (m_outlineColor == c) {
        return;
    }

    m_outlineColor = c;
    emit styleChanged();
}

QColor PolylinePointStyle::outlineColorHovered() const
{
    return m_outlineColorHovered;
}

void PolylinePointStyle::setOutlineColorHovered(const QColor& c)
{
    if (m_outlineColorHovered == c) {
        return;
    }

    m_outlineColorHovered = c;
    emit styleChanged();
}
