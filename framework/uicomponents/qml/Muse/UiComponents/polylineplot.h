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

#include <QColor>
#include <QPointF>
#include <QtQuick/QQuickPaintedItem>
#include <QVector>
#include <QtQmlIntegration/qqmlintegration.h>

#include "actions/actionable.h"
#include "async/asyncable.h"

#include "modularity/ioc.h"
#include "actions/iactionsdispatcher.h"
#include "ui/iuiconfiguration.h"

#include "internal/polylinepointstyle.h"

// NOTE: all of fooN() function are normalized, returning 0..1 values

namespace muse::uicomponents {
struct GhostPoint {
    QPointF point;
    qreal distToSegment = 1e18;
};

class PolylinePlot : public QQuickPaintedItem, public muse::async::Asyncable, public muse::actions::Actionable, public muse::Contextable
{
    Q_OBJECT

    Q_PROPERTY(PolylinePointStyle * standardPointStyle READ standardPointStyle CONSTANT)

    Q_PROPERTY(bool ghostPointsEnabled READ ghostPointsEnabled WRITE setGhostPointsEnabled NOTIFY ghostPointsEnabledChanged)
    Q_PROPERTY(PolylinePointStyle * ghostPointStyle READ ghostPointStyle CONSTANT)

    Q_PROPERTY(bool selectedPointsEnabled READ selectedPointsEnabled WRITE setSelectedPointsEnabled NOTIFY selectedPointsEnabledChanged)
    Q_PROPERTY(PolylinePointStyle * selectedPointStyle READ selectedPointStyle CONSTANT)

    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(bool drawBackground READ drawBackground WRITE setDrawBackground NOTIFY drawBackgroundChanged)
    Q_PROPERTY(qreal baselineN READ baselineN WRITE setBaselineN NOTIFY baselineNChanged)

    Q_PROPERTY(qreal hitRadius READ hitRadius WRITE setHitRadius NOTIFY hitRadiusChanged)

    Q_PROPERTY(bool isSnapEnabled READ isSnapEnabled WRITE setIsSnapEnabled NOTIFY isSnapEnabledChanged)
    Q_PROPERTY(qreal snapThresholdPx READ snapThresholdPx WRITE setSnapThresholdPx NOTIFY snapThresholdPxChanged)

    Q_PROPERTY(QVector<QPointF> points READ points WRITE setPoints NOTIFY pointsChanged)
    // indices in the colorsUnderLine vector relate to the lines between points (not the points themselves)
    Q_PROPERTY(QVector<QColor> colorsUnderLine READ colorsUnderLine WRITE setColorsUnderLine NOTIFY colorsUnderLineChanged)

    Q_PROPERTY(qreal defaultValue READ defaultValue WRITE setDefaultValue NOTIFY defaultValueChanged)

    Q_PROPERTY(qreal xRangeFrom READ xRangeFrom WRITE setXRangeFrom NOTIFY xRangeFromChanged)
    Q_PROPERTY(qreal xRangeTo READ xRangeTo WRITE setXRangeTo NOTIFY xRangeToChanged)

    Q_PROPERTY(qreal yRangeFrom READ yRangeFrom WRITE setYRangeFrom NOTIFY yRangeFromChanged)
    Q_PROPERTY(qreal yRangeTo READ yRangeTo WRITE setYRangeTo NOTIFY yRangeToChanged)
    Q_PROPERTY(qreal ySplitNormalized READ ySplitNormalized WRITE setYSplitNormalized NOTIFY ySplitNormalizedChanged)
    Q_PROPERTY(qreal ySplitValue READ ySplitValue WRITE setYSplitValue NOTIFY ySplitValueChanged)

    // TODO: dB or linear (separate setting for x and y axis)

    Q_PROPERTY(bool yAxisInverse READ yAxisInverse WRITE setYAxisInverse NOTIFY yAxisInverseChanged)

    Q_PROPERTY(bool hasActivePoint READ hasActivePoint NOTIFY activePointChanged)
    Q_PROPERTY(qreal activePointX READ activePointX NOTIFY activePointChanged)
    Q_PROPERTY(qreal activePointY READ activePointY NOTIFY activePointChanged)
    Q_PROPERTY(qreal activePointValue READ activePointValue NOTIFY activePointChanged)

    QML_ELEMENT

    muse::GlobalInject<muse::ui::IUiConfiguration> uiConfiguration;

    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher { this };

public:
    explicit PolylinePlot(QQuickItem* parent = nullptr);

    Q_INVOKABLE void init();

    PolylinePointStyle* standardPointStyle();
    PolylinePointStyle* ghostPointStyle();
    PolylinePointStyle* selectedPointStyle();

    bool ghostPointsEnabled() const;
    void setGhostPointsEnabled(bool);

    bool selectedPointsEnabled() const;
    void setSelectedPointsEnabled(bool);

    QColor lineColor() const;
    void setLineColor(const QColor&);

    qreal lineWidth() const;
    void setLineWidth(qreal);

    bool drawBackground() const;
    void setDrawBackground(bool);

    qreal baselineN() const;
    void setBaselineN(qreal);

    qreal hitRadius() const;
    void setHitRadius(qreal);

    bool isSnapEnabled() const;
    void setIsSnapEnabled(bool);

    qreal snapThresholdPx() const;
    void setSnapThresholdPx(qreal);

    QVector<QPointF> points() const;
    void setPoints(const QVector<QPointF>&);

    QVector<QColor> colorsUnderLine() const;
    void setColorsUnderLine(const QVector<QColor>&);

    qreal defaultValue() const;
    void setDefaultValue(qreal v);

    qreal xRangeFrom() const;
    void setXRangeFrom(qreal);

    qreal xRangeTo() const;
    void setXRangeTo(qreal);

    qreal yRangeFrom() const;
    void setYRangeFrom(qreal);

    qreal yRangeTo() const;
    void setYRangeTo(qreal);

    qreal ySplitNormalized() const;
    void setYSplitNormalized(qreal);

    qreal ySplitValue() const;
    void setYSplitValue(qreal);

    bool yAxisInverse() const;
    void setYAxisInverse(bool);

    bool hasActivePoint() const;
    qreal activePointX() const;
    qreal activePointY() const;
    qreal activePointValue() const;

    void geometryChange(const QRectF& newG, const QRectF& oldG) override;
    void paint(QPainter* painter) override;

signals:
    void ghostPointsEnabledChanged();
    void selectedPointsEnabledChanged();

    void lineColorChanged();
    void lineWidthChanged();
    void drawBackgroundChanged();
    void baselineNChanged();

    void hitRadiusChanged();
    void isSnapEnabledChanged();
    void snapThresholdPxChanged();

    void pointAdded(qreal x, qreal y, bool completed);
    void pointMoved(int index, qreal x, qreal y, bool completed);
    void pointRemoved(int index, bool completed);
    void dragCancelled();
    void interactionFinished();

    void xRangeFromChanged();
    void xRangeToChanged();
    void yRangeFromChanged();
    void yRangeToChanged();
    void ySplitNormalizedChanged();
    void ySplitValueChanged();
    void yAxisInverseChanged();

    void defaultValueChanged();
    void pointsNChanged();
    void pointsChanged();

    void colorsUnderLineChanged();

    void activePointChanged();

protected:
    void hoverMoveEvent(QHoverEvent* e) override;
    void hoverLeaveEvent(QHoverEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;

private:
    QVector<QPointF> polylinePx() const;
    bool isNearLinePx(const QPointF& px) const;
    GhostPoint ghostPointToPolylinePx(const QPointF& px) const;
    int pointIndexAtPx(const QPointF& px) const;

    void updateCursor();
    void resetGestureState();

    qreal clamp01(qreal v) const;
    QPointF clamp01(const QPointF& p) const;

    void rebuildVisiblePoints();
    QVector<QPointF> normalizedFromDomain(const QVector<QPointF>& pts) const;
    QVector<QPointF> domainFromNormalized(const QVector<QPointF>& ptsN) const;

    QPointF domainFromNormalized(const QPointF& pN) const;
    QPointF normalizedFromDomain(const QPointF& p) const;

    bool hasValidXRange() const;
    bool hasValidYRange() const;
    bool hasValidYSplit() const;

    qreal yDomainFromNormalized(qreal yNormalized) const;
    qreal yNormalizedFromDomain(qreal yDomain) const;
    void updateBaselineFromDefaultValue();

    QPointF snapToNeighbor(qreal dragPxX, QPointF pDomain) const;
    void updateActivePoint();

    void drawLinesAndFillUnder(QPainter* painter) const;
    void paintPoint(QPainter* painter, const PolylinePointStyle* style, const QPointF& centre, bool useHoveredStyle) const;

private:
    QColor m_lineColor;
    qreal m_lineWidth = 1.0;
    qreal m_baselineN = 0.5;

    PolylinePointStyle* m_standardPointStyle = nullptr;

    bool m_ghostPointsEnabled = true;
    PolylinePointStyle* m_ghostPointStyle = nullptr;

    bool m_selectedPointsEnabled = false;
    PolylinePointStyle* m_selectedPointStyle = nullptr;

    qreal m_hitRadius = 9.0;
    bool m_isSnapEnabled = true;
    qreal m_snapThresholdPx = 7.0;

    QVector<QPointF> m_points;          // domain points as provided from model
    QVector<QPointF> m_pointsNVisible;  // normalized points [0..1], cropped to frame boundaries (used for drawing only)

    // mapping for m_pointsNVisible -> index in m_points
    QVector<int> m_visibleToDomainIndex;

    QVector<QColor> m_colorsUnderLine;

    qreal m_defaultValue = 1.0;

    qreal m_xFrom = 0.0;
    qreal m_xTo   = 1.0;
    qreal m_yFrom = 0.0;
    qreal m_yTo   = 1.0;
    qreal m_ySplitNormalized = 1.0;
    qreal m_ySplitValue = 1.0;
    bool m_yAxisInverse = true;

    bool m_drawBackground = true;

    bool m_hoveredOnLine = false;
    QPointF m_hoverPx;
    QPointF m_hoverGhostPx;

    std::unordered_set<int> m_selectedPointsIndices;

    bool m_pressedOnLine = false;
    bool m_pressed = false;
    QPointF m_pressPx;
    bool m_pressedOnPoint = false;
    int m_pressedPointIndex = -1;
    bool m_hasDraggedPointDomain = false;
    QPointF m_draggedPointDomain;

    bool m_movedSincePress = false;

    bool m_hasActivePoint = false;
    QPointF m_activePointPx;
    qreal m_activePointValue = 0.0;
};
}
