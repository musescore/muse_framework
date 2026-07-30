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

#include <QObject>
#include <QColor>

namespace muse::uicomponents {
class PolylinePointStyle : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal centerRadius READ centerRadius WRITE setCenterRadius NOTIFY styleChanged)
    Q_PROPERTY(QColor centerColor READ centerColor WRITE setCenterColor NOTIFY styleChanged)
    Q_PROPERTY(qreal centerRadiusHovered READ centerRadiusHovered WRITE setCenterRadiusHovered NOTIFY styleChanged)
    Q_PROPERTY(QColor centerColorHovered READ centerColorHovered WRITE setCenterColorHovered NOTIFY styleChanged)

    Q_PROPERTY(qreal middleRingWidth READ middleRingWidth WRITE setMiddleRingWidth NOTIFY styleChanged)
    Q_PROPERTY(QColor middleRingColor READ middleRingColor WRITE setMiddleRingColor NOTIFY styleChanged)
    Q_PROPERTY(qreal middleRingWidthHovered READ middleRingWidthHovered WRITE setMiddleRingWidthHovered NOTIFY styleChanged)
    Q_PROPERTY(QColor middleRingColorHovered READ middleRingColorHovered WRITE setMiddleRingColorHovered NOTIFY styleChanged)

    Q_PROPERTY(qreal outlineWidth READ outlineWidth WRITE setOutlineWidth NOTIFY styleChanged)
    Q_PROPERTY(QColor outlineColor READ outlineColor WRITE setOutlineColor NOTIFY styleChanged)
    Q_PROPERTY(qreal outlineWidthHovered READ outlineWidthHovered WRITE setOutlineWidthHovered NOTIFY styleChanged)
    Q_PROPERTY(QColor outlineColorHovered READ outlineColorHovered WRITE setOutlineColorHovered NOTIFY styleChanged)

public:
    explicit PolylinePointStyle(QObject* parent = nullptr);

    qreal centerRadius() const;
    void setCenterRadius(qreal);
    qreal centerRadiusHovered() const;
    void setCenterRadiusHovered(qreal);

    QColor centerColor() const;
    void setCenterColor(const QColor&);
    QColor centerColorHovered() const;
    void setCenterColorHovered(const QColor&);

    qreal middleRingWidth() const;
    void setMiddleRingWidth(qreal);
    qreal middleRingWidthHovered() const;
    void setMiddleRingWidthHovered(qreal);

    QColor middleRingColor() const;
    void setMiddleRingColor(const QColor&);
    QColor middleRingColorHovered() const;
    void setMiddleRingColorHovered(const QColor&);

    qreal outlineWidth() const;
    void setOutlineWidth(qreal);
    qreal outlineWidthHovered() const;
    void setOutlineWidthHovered(qreal);

    QColor outlineColor() const;
    void setOutlineColor(const QColor&);
    QColor outlineColorHovered() const;
    void setOutlineColorHovered(const QColor&);

signals:
    void styleChanged();

private:
    qreal m_centerRadius = 3.0;
    QColor m_centerColor;
    qreal m_centerRadiusHovered = 3.0;
    QColor m_centerColorHovered;

    qreal m_middleRingWidth = 0.0;
    QColor m_middleRingColor;
    qreal m_middleRingWidthHovered = 0.0;
    QColor m_middleRingColorHovered;

    qreal m_outlineWidth = 0.0;
    QColor m_outlineColor;
    qreal m_outlineWidthHovered = 0.0;
    QColor m_outlineColorHovered;
};
}
