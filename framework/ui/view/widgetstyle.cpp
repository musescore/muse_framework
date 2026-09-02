/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "widgetstyle.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QGroupBox>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTextEdit>
#include <QToolBar>
#include <QVariant>

#include "api/themeapi.h"
#include "iconcodes.h"

#include "log.h"

using namespace muse::ui;
using muse::api::ThemeApi;

static const QPen NO_BORDER(Qt::transparent, 0);
static const QBrush NO_FILL(Qt::transparent);
static const int DEFAULT_RADIUS = 3;

static const int GROUP_BOX_LABEL_SPACING = 2;

//! In QML, a border is drawn _inside_ a rectangle.
//! In C++, a border would normally be drawn half inside the rectangle, half outside.
//! In this function, we exactly replicate the behaviour from QML.
static void drawRoundedRect(QPainter* painter, const QRectF& rect, const qreal radius, const QBrush& brush = NO_FILL,
                            const QPen& pen = NO_BORDER)
{
    IF_ASSERT_FAILED(painter) {
        return;
    }

    const qreal bw = pen.width();

    if (bw <= 0 || pen.color().alpha() == 0) {
        if (brush == NO_FILL) {
            return;
        }

        painter->save();
        painter->setPen(NO_BORDER);
        painter->setBrush(brush);
        painter->drawRoundedRect(rect, radius, radius);
        painter->restore();
        return;
    }

    painter->save();
    painter->setPen(NO_BORDER);
    painter->setBrush(brush);
    painter->drawRoundedRect(rect.adjusted(bw, bw, -bw, -bw), radius - bw, radius - bw);

    const qreal corr = 0.5 * bw;
    painter->setPen(pen);
    painter->setBrush(NO_FILL);
    painter->drawRoundedRect(rect.adjusted(corr, corr, -corr, -corr), radius - corr, radius - corr);
    painter->restore();
}

WidgetStyle::WidgetStyle(std::shared_ptr<ThemeApi> t)
    : QProxyStyle(QStyleFactory::create("Fusion")), m_theme(t)
{
}

void WidgetStyle::init()
{
    connect(m_theme.get(), &ThemeApi::themeChanged, this, &WidgetStyle::update);

    update();
}

void WidgetStyle::update()
{
    QColor fontPrimaryColorDisabled = m_theme->fontPrimaryColor();
    fontPrimaryColorDisabled.setAlphaF(m_theme->itemOpacityDisabled());

    QColor linkColorDisabled = m_theme->linkColor();
    linkColorDisabled.setAlphaF(m_theme->itemOpacityDisabled());

    QColor backgroundPrimaryColorDisabled = m_theme->backgroundPrimaryColor();
    backgroundPrimaryColorDisabled.setAlphaF(m_theme->itemOpacityDisabled());

    QColor backgroundSecondaryColorDisabled = m_theme->backgroundSecondaryColor();
    backgroundSecondaryColorDisabled.setAlphaF(m_theme->itemOpacityDisabled());

    QColor buttonColorDisabled = m_theme->buttonColor();
    buttonColorDisabled.setAlphaF(m_theme->itemOpacityDisabled());

    QPalette palette(QApplication::palette());
    palette.setColor(QPalette::Window, m_theme->backgroundPrimaryColor());
    palette.setColor(QPalette::Disabled, QPalette::Window, backgroundPrimaryColorDisabled);
    palette.setColor(QPalette::WindowText, m_theme->fontPrimaryColor());
    palette.setColor(QPalette::Disabled, QPalette::WindowText, fontPrimaryColorDisabled);

    palette.setColor(QPalette::Base, m_theme->backgroundSecondaryColor());
    palette.setColor(QPalette::Disabled, QPalette::Base, backgroundSecondaryColorDisabled);
    palette.setColor(QPalette::AlternateBase, m_theme->backgroundSecondaryColor());
    palette.setColor(QPalette::Disabled, QPalette::AlternateBase, backgroundSecondaryColorDisabled);

    palette.setColor(QPalette::Text, m_theme->fontPrimaryColor());
    palette.setColor(QPalette::Disabled, QPalette::Text, fontPrimaryColorDisabled);

    palette.setColor(QPalette::Link, m_theme->linkColor());
    palette.setColor(QPalette::Disabled, QPalette::Link, linkColorDisabled);

    palette.setColor(QPalette::Button, m_theme->buttonColor());
    palette.setColor(QPalette::Disabled, QPalette::Button, buttonColorDisabled);
    palette.setColor(QPalette::ButtonText, m_theme->fontPrimaryColor());
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, fontPrimaryColorDisabled);

    palette.setColor(QPalette::ToolTipBase, m_theme->popupBackgroundColor());
    palette.setColor(QPalette::ToolTipText, m_theme->fontPrimaryColor());

    palette.setColor(QPalette::Highlight, m_theme->accentColor());
    palette.setColor(QPalette::HighlightedText, m_theme->fontPrimaryColor());

    palette.setColor(QPalette::PlaceholderText, m_theme->fontPrimaryColor());
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, fontPrimaryColorDisabled);

    QApplication::setStyle(this);
    QApplication::setPalette(palette);

    QFont font = m_theme->bodyFont();
    QString styleSheet = QString("* { font: %1px \"%2\" } ")
                         .arg(QString::number(font.pixelSize()), font.family());
    qApp->setStyleSheet(styleSheet);
}

void WidgetStyle::polish(QWidget* widget)
{
    QProxyStyle::polish(widget);

    if (qobject_cast<QAbstractItemView*>(widget)
        || qobject_cast<QGroupBox*>(widget)
        || qobject_cast<QLineEdit*>(widget)) {
        // Make hovering work
        widget->setMouseTracking(true);
        widget->setAttribute(Qt::WA_Hover, true);
    }
}

void WidgetStyle::unpolish(QWidget* widget)
{
    QProxyStyle::unpolish(widget);

    if (qobject_cast<QAbstractItemView*>(widget)
        || qobject_cast<QGroupBox*>(widget)
        || qobject_cast<QLineEdit*>(widget)) {
        widget->setMouseTracking(false);
        widget->setAttribute(Qt::WA_Hover, false);
    }
}

void WidgetStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option, QPainter* painter,
                                const QWidget* widget) const
{
    StyleState styleState;
    styleState.enabled = option->state & State_Enabled;
    styleState.hovered = option->state & State_MouseOver;
    styleState.pressed = option->state & State_Sunken;
    styleState.focused = (option->state & State_KeyboardFocusChange) && (option->state & State_HasFocus);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    //! NOTE This drawing code is based on the implementation in QML.
    switch (element) {
    // Buttons (and ComboBoxes)
    case QStyle::PE_PanelButtonCommand: {
        auto buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
        const bool accentButton = (buttonOption && buttonOption->features & QStyleOptionButton::DefaultButton)
                                  || option->state & State_On;
        const bool flat = (buttonOption && buttonOption->features & QStyleOptionButton::Flat)
                          && !(option->state & State_On);

        QColor paletteColor = widget ? widget->palette().color(QPalette::Button) : QColor();
        const QColor background = paletteColor.isValid() ? paletteColor : m_theme->buttonColor();

        drawButtonBackground(painter, option->rect, styleState, accentButton, flat, background);
    } break;
    case QStyle::PE_FrameDefaultButton: {
        auto buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
        const bool flat = (buttonOption && buttonOption->features & QStyleOptionButton::Flat)
                          && !(option->state & State_On);
        if (flat && styleState.focused) {
            // For other buttons, this is done in `drawButtonBackground`, but that is not called for flat buttons
            drawRoundedRect(painter, option->rect, DEFAULT_RADIUS, NO_FILL,
                            QPen(m_theme->fontPrimaryColor(), m_theme->navCtrlBorderWidth()));
        }
    } break;

    // Checkboxes
    case QStyle::PE_IndicatorCheckBox: {
        const bool indeterminate = option->state & State_NoChange;
        const bool checked = option->state & State_On;
        const bool inMenu = qobject_cast<const QMenu*>(widget);

        drawCheckboxIndicator(painter, option->rect, styleState, checked, indeterminate, inMenu);
    } break;

    // Radio buttons
    case QStyle::PE_IndicatorRadioButton: {
        const bool selected = option->state & State_On;

        drawRadioButtonIndicator(painter, option->rect, styleState, selected);
    } break;

    case QStyle::PE_FrameLineEdit: {
        const bool editing = option->state & State_HasFocus;

        drawLineEditBackground(painter, option->rect, styleState, editing);
    } break;

    case QStyle::PE_FrameFocusRect: {
        bool isTreeWidget = option->styleObject && option->styleObject->inherits("QTreeWidget");
        if (isTreeWidget) {
            drawRoundedRect(painter, option->rect, 1, NO_FILL, QPen(m_theme->fontPrimaryColor(), m_theme->navCtrlBorderWidth()));
        }

        //! NOTE: need for removing frame focus rectangle
    } break;

    // Indicator icons
    case QStyle::PE_IndicatorSpinUp:
    case QStyle::PE_IndicatorSpinDown:
    case QStyle::PE_IndicatorSpinPlus:
    case QStyle::PE_IndicatorSpinMinus: {
        drawIndicatorIcon(painter, option->rect, styleState, element);
    } break;

    // ViewItem
    case QStyle::PE_PanelItemViewItem: {
        bool selected = option->state & State_Selected;

        drawViewItemBackground(painter, option->rect, styleState, selected);
    } break;

    // Toolbar
    case QStyle::PE_PanelToolBar: {
        painter->fillRect(option->rect, m_theme->backgroundPrimaryColor());
    } break;
    case QStyle::PE_IndicatorToolBarHandle: {
        drawToolbarGrip(painter, option->rect, option->state & State_Horizontal);
    } break;

    // GroupBox
    case QStyle::PE_FrameGroupBox: {
        drawRoundedRect(painter, option->rect, DEFAULT_RADIUS, QBrush(QColor::fromRgba(0x03000000)),
                        QPen(m_theme->strokeColor(), fmax(m_theme->borderWidth(), 1.0)));
    } break;

    // Menu
    case QStyle::PE_PanelMenu: {
        drawRoundedRect(painter, option->rect, 1, m_theme->backgroundPrimaryColor(), QPen(m_theme->strokeColor(), m_theme->borderWidth()));
    } break;
    case QStyle::PE_FrameMenu: {
        drawRoundedRect(painter, option->rect, 1, NO_FILL, QPen(m_theme->strokeColor(), m_theme->borderWidth()));
    } break;

    case QStyle::PE_Frame: {
        if (qobject_cast<const QTextEdit*>(widget) != nullptr) {
            if (styleState.enabled) {
                drawRoundedRect(painter, option->rect, DEFAULT_RADIUS, NO_FILL, QPen(m_theme->strokeColor(), m_theme->borderWidth()));
            } else {
                QColor penBorderColor = m_theme->strokeColor();
                penBorderColor.setAlphaF(m_theme->itemOpacityDisabled());
                drawRoundedRect(painter, option->rect, DEFAULT_RADIUS, NO_FILL, QPen(penBorderColor, m_theme->borderWidth()));
            }
        }
    } break;

    default:
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        break;
    }

    painter->restore();
}

void WidgetStyle::drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex* option, QPainter* painter,
                                     const QWidget* widget) const
{
    StyleState styleState;
    styleState.enabled = option->state & State_Enabled;
    styleState.hovered = option->state & State_MouseOver;
    styleState.pressed = option->state & State_Sunken;
    styleState.focused = (option->state & State_KeyboardFocusChange) && (option->state & State_HasFocus);

    switch (control) {
    case CC_ScrollBar: {
        QProxyStyle::drawComplexControl(control, option, painter, widget);

        if (m_theme->configuration()->isHighContrast()) {
            QRect scrollBarHandle = QProxyStyle::subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);

            QColor handleColor = m_theme->fontPrimaryColor();
            handleColor.setAlphaF(
                !styleState.enabled ? m_theme->buttonOpacityNormal()
                * m_theme->itemOpacityDisabled() : styleState.pressed ? m_theme->buttonOpacityHit() : styleState.hovered ? m_theme->buttonOpacityHover() : m_theme->buttonOpacityNormal());

            drawRoundedRect(painter, option->rect, 1, NO_FILL, QPen(m_theme->strokeColor(), m_theme->borderWidth()));
            drawRoundedRect(painter, scrollBarHandle, 1, handleColor, NO_BORDER);
        }
    } break;

    case CC_SpinBox: {
        QProxyStyle::drawComplexControl(control, option, painter, widget);

        QRect spinBoxFrame = QProxyStyle::subControlRect(CC_SpinBox, option, SC_SpinBoxFrame, widget);
        if (styleState.focused) {
            drawRoundedRect(painter, spinBoxFrame, DEFAULT_RADIUS, NO_FILL,
                            QPen(m_theme->fontPrimaryColor(), m_theme->navCtrlBorderWidth()));
        }
    } break;

    default: QProxyStyle::drawComplexControl(control, option, painter, widget);
    }
}

QRect WidgetStyle::subControlRect(QStyle::ComplexControl control, const QStyleOptionComplex* option, QStyle::SubControl subControl,
                                  const QWidget* widget) const
{
    //QRect commonStyleRect = QCommonStyle::subControlRect(control, option, subControl, widget);
    QRect proxyStyleRect = QProxyStyle::subControlRect(control, option, subControl, widget);

    switch (control) {
    case QStyle::CC_GroupBox:
        if (const QStyleOptionGroupBox* optionGroupBox = qstyleoption_cast<const QStyleOptionGroupBox*>(option)) {
            int indicatorWidth = 0;
            int indicatorHeight = 0;
            int indicatorSpacing = 0;
            const QSize textSize = option->fontMetrics.boundingRect(optionGroupBox->text).size() + QSize(2, 2);

            const bool checkable = option->subControls & QStyle::SC_GroupBoxCheckBox;

            if (checkable) {
                const bool isRadioButtonGroupBox
                    =widget && strcmp(widget->metaObject()->className(), "muse::uicomponents::RadioButtonGroupBox") == 0;

                indicatorWidth = pixelMetric(isRadioButtonGroupBox ? PM_ExclusiveIndicatorWidth : PM_IndicatorWidth, option, widget);
                indicatorHeight = pixelMetric(isRadioButtonGroupBox ? PM_ExclusiveIndicatorHeight : PM_IndicatorHeight, option, widget);
                indicatorSpacing
                    = pixelMetric(isRadioButtonGroupBox ? PM_RadioButtonLabelSpacing : PM_CheckBoxLabelSpacing, option, widget);
            }

            if (subControl == SC_GroupBoxFrame) {
                int topMargin = std::max(indicatorHeight, textSize.height()) + GROUP_BOX_LABEL_SPACING;
                return option->rect.adjusted(0, topMargin, 0, 0);
            }

            if (subControl == SC_GroupBoxContents) {
                int margin = 3;
                int topMargin = margin + std::max(indicatorHeight, textSize.height()) + GROUP_BOX_LABEL_SPACING;
                return option->rect.adjusted(margin, topMargin, -margin, -margin);
            }

            const int width = textSize.width() + (checkable ? indicatorWidth + indicatorSpacing : 0);
            QRect rect;

            if (option->rect.width() > width) {
                switch (optionGroupBox->textAlignment & Qt::AlignHorizontal_Mask) {
                case Qt::AlignHCenter:
                    rect.moveLeft((option->rect.width() - width) / 2);
                    break;
                case Qt::AlignRight:
                    rect.moveLeft(option->rect.width() - width);
                    break;
                }
            }

            if (subControl == SC_GroupBoxCheckBox) {
                rect.setWidth(indicatorWidth);
                rect.setHeight(indicatorHeight);
                rect.moveTop(textSize.height() > indicatorHeight ? (textSize.height() - indicatorHeight) / 2 : 0);
            } else if (subControl == SC_GroupBoxLabel) {
                rect.setSize(textSize);
                if (checkable) {
                    rect.moveTop(textSize.height() < indicatorHeight ? (indicatorHeight - textSize.height()) / 2 : 0);
                    rect.translate(indicatorWidth + indicatorSpacing, 0);
                }
            }

            return visualRect(option->direction, option->rect, rect);
        }
        break;
    default:
        break;
    }

    return proxyStyleRect;
}

int WidgetStyle::pixelMetric(QStyle::PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
    //! NOTE These metrics are based on the implementation in QML.
    switch (metric) {
    case PM_IndicatorWidth: // Checkbox
    case PM_IndicatorHeight:
    case PM_ExclusiveIndicatorWidth: // Radio button
    case PM_ExclusiveIndicatorHeight:
        return 20;
    case PM_CheckBoxLabelSpacing: // Checkbox
    case PM_RadioButtonLabelSpacing: // Radio button
        return 6;
    case PM_ToolBarHandleExtent: // Toolbars
        return 32;
    default:
        break;
    }

    return QProxyStyle::pixelMetric(metric, option, widget);
}

QSize WidgetStyle::sizeFromContents(QStyle::ContentsType type, const QStyleOption* option, const QSize& contentsSize,
                                    const QWidget* widget) const
{
    QSize commonStyleSize = QCommonStyle::sizeFromContents(type, option, contentsSize, widget);
    QSize proxyStyleSize = QProxyStyle::sizeFromContents(type, option, contentsSize, widget);

    //! NOTE These calculations are based on the implementation in QML.
    switch (type) {
    case CT_PushButton:
        return QSize(std::max(contentsSize.width() + 32, 132),
                     contentsSize.height() + 14);
    case CT_ToolButton:
        return contentsSize.expandedTo(QSize(30, 30));
    case CT_ComboBox:
    case CT_LineEdit:
        return proxyStyleSize.expandedTo(QSize(30, 30));
    case CT_SpinBox:
        return QSize(proxyStyleSize.width(), 26); // results in a height of 30
    case CT_GroupBox: {
        const QGroupBox* groupBox = qobject_cast<const QGroupBox*>(widget);
        const bool checkable = groupBox && groupBox->isCheckable();

        if (checkable) {
            const bool isRadioButtonGroupBox
                =widget && strcmp(widget->metaObject()->className(), "muse::uicomponents::RadioButtonGroupBox") == 0;

            int pm = pixelMetric(isRadioButtonGroupBox ? PM_ExclusiveIndicatorHeight : PM_IndicatorHeight, option, widget);
            return commonStyleSize + QSize(0, std::max(pm, option->fontMetrics.height()));
        }

        return commonStyleSize + QSize(0, option->fontMetrics.height());
    } break;
    case CT_ItemViewItem:
        return commonStyleSize.expandedTo(QSize(20, 20));
    case CT_MenuItem:
        if (const QStyleOptionMenuItem* optionMenuItem = qstyleoption_cast<const QStyleOptionMenuItem*>(option)) {
            if (optionMenuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                return proxyStyleSize;
            }
        }
        return proxyStyleSize.expandedTo(QSize(30, 30));
    default:
        break;
    }

    return proxyStyleSize;
}

QIcon WidgetStyle::standardIcon(QStyle::StandardPixmap standardIcon, const QStyleOption* option, const QWidget* widget) const
{
    switch (standardIcon) {
    case SP_DialogOkButton:
    case SP_DialogCancelButton:
    case SP_DialogHelpButton:
    case SP_DialogOpenButton:
    case SP_DialogSaveButton:
    case SP_DialogCloseButton:
    case SP_DialogApplyButton:
    case SP_DialogResetButton:
    case SP_DialogDiscardButton:
    case SP_DialogYesButton:
    case SP_DialogNoButton:
    case SP_DialogYesToAllButton:
    case SP_DialogNoToAllButton:
    case SP_DialogSaveAllButton:
    case SP_DialogAbortButton:
    case SP_DialogRetryButton:
    case SP_DialogIgnoreButton:
        return {};
    default:
        return QProxyStyle::standardIcon(standardIcon, option, widget);
    }
}

int WidgetStyle::styleHint(QStyle::StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const
{
    switch (hint) {
    case SH_DitherDisabledText:
    case SH_EtchDisabledText:
        return false;
    case SH_ItemView_ScrollMode:
        return QAbstractItemView::ScrollPerPixel;
    default:
        break;
    }

    return QProxyStyle::styleHint(hint, option, widget, returnData);
}

// ====================================================
// QStyle elements drawing
// ====================================================

void WidgetStyle::drawButtonBackground(QPainter* painter, const QRect& rect, const StyleState& styleState, bool accentButton, bool flat,
                                       const QColor& defaultBackground) const
{
    QColor backgroundColor(accentButton ? m_theme->accentColor() : defaultBackground);

    if (styleState.enabled) {
        backgroundColor.setAlphaF(styleState.pressed ? m_theme->buttonOpacityHit()
                                  : styleState.hovered ? m_theme->buttonOpacityHover()
                                  : !flat ? m_theme->buttonOpacityNormal()
                                  : 0.0);

        if (m_theme->configuration()->isHighContrast()) {
            QColor penBorderColor(m_theme->strokeColor());
            penBorderColor.setAlphaF(styleState.pressed ? m_theme->buttonOpacityHit()
                                     : styleState.hovered ? m_theme->buttonOpacityHover()
                                     : m_theme->buttonOpacityNormal());

            drawRoundedRect(painter, rect, DEFAULT_RADIUS, backgroundColor, QPen(penBorderColor, m_theme->borderWidth()));
        } else {
            drawRoundedRect(painter, rect, DEFAULT_RADIUS, backgroundColor, NO_BORDER);
        }
    } else {
        backgroundColor.setAlphaF(flat ? 0.0 : m_theme->buttonOpacityNormal() * m_theme->itemOpacityDisabled());

        drawRoundedRect(painter, rect, DEFAULT_RADIUS, backgroundColor, NO_BORDER);
    }

    if (styleState.focused) {
        drawRoundedRect(painter, rect, DEFAULT_RADIUS, NO_FILL, QPen(m_theme->fontPrimaryColor(), m_theme->navCtrlBorderWidth()));
    }
}

void WidgetStyle::drawCheckboxIndicator(QPainter* painter, const QRect& rect, const StyleState& styleState, bool checked,
                                        bool indeterminate,
                                        bool inMenu) const
{
    QColor backgroundColor = m_theme->buttonColor();
    const qreal borderRadius = 2;

    if (!inMenu) {
        backgroundColor.setAlphaF(!styleState.enabled ? m_theme->buttonOpacityNormal() * m_theme->itemOpacityDisabled()
                                  : styleState.pressed ? m_theme->buttonOpacityHit()
                                  : styleState.hovered ? m_theme->buttonOpacityHover()
                                  : m_theme->buttonOpacityNormal());

        QColor penBorderColor(Qt::transparent);
        int penBorderWidth = 0;
        if (m_theme->configuration()->isHighContrast()) {
            penBorderColor = m_theme->strokeColor();
            penBorderWidth = styleState.enabled ? m_theme->borderWidth() : 0;
        } else {
            penBorderWidth = styleState.enabled && (styleState.hovered || styleState.pressed) ? m_theme->borderWidth() : 0;
        }

        penBorderColor.setAlphaF(
            styleState.pressed ? m_theme->buttonOpacityHit() : styleState.hovered ? m_theme->buttonOpacityHover() : m_theme->buttonOpacityNormal());
        drawRoundedRect(painter, rect, borderRadius, backgroundColor, QPen(penBorderColor, penBorderWidth));
    }

    if (styleState.focused) {
        drawRoundedRect(painter, rect, borderRadius, NO_FILL, QPen(m_theme->fontPrimaryColor(), m_theme->navCtrlBorderWidth()));
    }

    if (checked || indeterminate) {
        QColor tickColor = m_theme->fontPrimaryColor();
        if (!styleState.enabled) {
            tickColor.setAlphaF(m_theme->itemOpacityDisabled());
        }
        painter->setPen(tickColor);
        painter->setFont(m_theme->iconsFont());
        painter->drawText(rect, Qt::AlignCenter,
                          iconCodeToChar(indeterminate ? IconCode::Code::MINUS : IconCode::Code::TICK_RIGHT_ANGLE));
    }
}

void WidgetStyle::drawRadioButtonIndicator(QPainter* painter, const QRect& rect, const StyleState& styleState, bool selected) const
{
    QColor borderColor = m_theme->fontPrimaryColor();
    QColor backgroundColor = m_theme->textFieldColor();

    if (!styleState.enabled) {
        borderColor.setAlphaF(m_theme->itemOpacityDisabled());
    } else if (styleState.pressed) {
        borderColor.setAlphaF(m_theme->buttonOpacityHit());
    } else if (styleState.hovered) {
        borderColor.setAlphaF(m_theme->buttonOpacityHover());
    } else {
        borderColor.setAlphaF(m_theme->buttonOpacityNormal());
    }

    const int borderWidth = 1;
    qreal outerCircleRadius = 10; // diameter = 20

    if (styleState.focused) {
        const qreal focusCircleRadius = outerCircleRadius;
        const QRect focusCircleRect(rect.center() + QPoint(1, 1) - QPoint(focusCircleRadius, focusCircleRadius),
                                    QSize(focusCircleRadius, focusCircleRadius) * 2);
        drawRoundedRect(painter, focusCircleRect, focusCircleRadius, NO_FILL,
                        QPen(m_theme->fontPrimaryColor(), m_theme->navCtrlBorderWidth()));

        outerCircleRadius -= m_theme->navCtrlBorderWidth();
    }

    const QRect outerCircleRect(rect.center() + QPoint(1, 1) - QPoint(outerCircleRadius, outerCircleRadius),
                                QSize(outerCircleRadius, outerCircleRadius) * 2);
    drawRoundedRect(painter, outerCircleRect, outerCircleRadius, backgroundColor, QPen(borderColor, borderWidth));

    if (selected || styleState.pressed) {
        QColor centerColor = m_theme->accentColor();
        const int innerCircleRadius = 5; // diameter = 10
        const QRect innerCircleRect(rect.center() + QPoint(1, 1) - QPoint(innerCircleRadius, innerCircleRadius),
                                    QSize(innerCircleRadius, innerCircleRadius) * 2);
        drawRoundedRect(painter, innerCircleRect, innerCircleRadius, centerColor);
    }
}

void WidgetStyle::drawLineEditBackground(QPainter* painter, const QRect& rect, const StyleState& styleState, bool editing) const
{
    QColor backgroundColor = m_theme->textFieldColor();
    backgroundColor.setAlphaF(!styleState.enabled ? m_theme->itemOpacityDisabled() : (editing ? 1 : (styleState.hovered ? 0.6 : 1)));

    if (styleState.focused) {
        drawRoundedRect(painter, rect, DEFAULT_RADIUS, NO_FILL, QPen(m_theme->fontPrimaryColor(), m_theme->navCtrlBorderWidth()));
    }

    QRect editRect = rect.adjusted(m_theme->navCtrlBorderWidth(), m_theme->navCtrlBorderWidth(),
                                   -m_theme->navCtrlBorderWidth(), -m_theme->navCtrlBorderWidth());
    QColor penBorderColor = editing ? m_theme->accentColor() : m_theme->strokeColor();
    penBorderColor.setAlphaF(editing ? 1 : (styleState.hovered ? 0.6 : 1));
    int borderWidth = m_theme->configuration()->isHighContrast() ? m_theme->borderWidth() : 1;

    if (styleState.enabled) {
        drawRoundedRect(painter, editRect, DEFAULT_RADIUS, backgroundColor, QPen(penBorderColor, borderWidth));
    } else {
        drawRoundedRect(painter, editRect, DEFAULT_RADIUS, backgroundColor, QPen(penBorderColor, 1));
    }
}

void WidgetStyle::drawIndicatorIcon(QPainter* painter, const QRect& rect, const StyleState& styleState,
                                    QStyle::PrimitiveElement element) const
{
    QColor color = m_theme->fontPrimaryColor();
    if (!styleState.enabled) {
        color.setAlphaF(m_theme->itemOpacityDisabled());
    }
    painter->setPen(color);
    painter->setFont(m_theme->iconsFont());

    IconCode::Code code;
    switch (element) {
    case QStyle::PE_IndicatorSpinPlus:
        code = IconCode::Code::PLUS;
        break;
    case QStyle::PE_IndicatorSpinMinus:
        code = IconCode::Code::MINUS;
        break;
    case QStyle::PE_IndicatorSpinUp:
        code = IconCode::Code::SMALL_ARROW_UP;
        break;
    case QStyle::PE_IndicatorSpinDown:
        code = IconCode::Code::SMALL_ARROW_DOWN;
        break;
    default:
        return;
    }

    painter->drawText(rect, Qt::AlignCenter, iconCodeToChar(code));
    drawRoundedRect(painter, rect, 1, NO_FILL, QPen(m_theme->strokeColor(), m_theme->borderWidth())); //does nothing apparently
}

void WidgetStyle::drawViewItemBackground(QPainter* painter, const QRect& rect, const StyleState& styleState, bool selected) const
{
    QColor backgroundColor(Qt::transparent);
    if (selected) {
        backgroundColor = m_theme->accentColor();
        backgroundColor.setAlphaF(
            styleState.enabled ? m_theme->accentOpacityHit() : m_theme->accentOpacityHit() * m_theme->itemOpacityDisabled());
    } else if (styleState.enabled && styleState.pressed) {
        backgroundColor = m_theme->buttonColor();
        backgroundColor.setAlphaF(m_theme->buttonOpacityHit());
    } else if (styleState.enabled && styleState.hovered) {
        backgroundColor = m_theme->buttonColor();
        backgroundColor.setAlphaF(m_theme->buttonOpacityHover());
    }

    painter->fillRect(rect, backgroundColor);

    if (m_theme->configuration()->isHighContrast()) {
        drawRoundedRect(painter, rect, 1, NO_FILL, QPen(m_theme->strokeColor(), m_theme->borderWidth()));
    }

    if (styleState.focused) {
        drawRoundedRect(painter, rect, 1, NO_FILL, QPen(m_theme->fontPrimaryColor(), m_theme->navCtrlBorderWidth()));
    }
}

void WidgetStyle::drawToolbarGrip(QPainter* painter, const QRect& rect, bool horizontal) const
{
    QColor gripColor(m_theme->fontPrimaryColor());
    gripColor.setAlphaF(0.5);
    painter->setPen(gripColor);
    painter->setFont(m_theme->iconsFont());

    int rotation = horizontal ? 0 : 90;
    QRect r = horizontal ? rect : QRect(rect.topLeft() + QPoint(0, -rect.width()),
                                        rect.size().transposed());
    painter->rotate(rotation);
    painter->drawText(r, Qt::AlignCenter, iconCodeToChar(IconCode::Code::TOOLBAR_GRIP));
    painter->rotate(-rotation);
}
