/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

#include "themeapi.h"

#include <QFontMetrics>

using namespace muse::ui;
using namespace muse::api;

struct FontConfig
{
    QFont::Weight weight = QFont::Normal;
    FontSizeType sizeType = FontSizeType::BODY;
};

ThemeApi::ThemeApi(IApiEngine* e)
    : api::ApiObject(e)
{
    setObjectName("UiTheme");
}

void ThemeApi::init()
{
    configuration()->currentThemeChanged().onNotify(this, [this]() {
        update();
    });

    initThemeValues();

    initUiFonts();
    initIconsFont();
    initMusicalFont();
    initMusicalTextFont();
    calculateDefaultButtonSize();
}

void ThemeApi::initThemeValues()
{
    QMap<ThemeStyleKey, QVariant> themeValues = configuration()->currentTheme().values;

    m_backgroundPrimaryColor = themeValues[BACKGROUND_PRIMARY_COLOR].toString();
    m_backgroundSecondaryColor = themeValues[BACKGROUND_SECONDARY_COLOR].toString();
    m_backgroundTertiaryColor = themeValues[BACKGROUND_TERTIARY_COLOR].toString();
    m_backgroundQuarternaryColor = themeValues[BACKGROUND_QUARTERNARY_COLOR].toString();
    m_popupBackgroundColor = themeValues[POPUP_BACKGROUND_COLOR].toString();
    m_textFieldColor = themeValues[TEXT_FIELD_COLOR].toString();
    m_accentColor = themeValues[ACCENT_COLOR].toString();
    m_strokeColor = themeValues[STROKE_COLOR].toString();
    m_buttonColor = themeValues[BUTTON_COLOR].toString();
    m_fontPrimaryColor = themeValues[FONT_PRIMARY_COLOR].toString();
    m_fontSecondaryColor = themeValues[FONT_SECONDARY_COLOR].toString();
    m_linkColor = themeValues[LINK_COLOR].toString();
    m_focusColor = themeValues[FOCUS_COLOR].toString();

    m_borderWidth = themeValues[BORDER_WIDTH].toReal();
    m_navCtrlBorderWidth = themeValues[NAVIGATION_CONTROL_BORDER_WIDTH].toReal();
    m_accentOpacityNormal = themeValues[ACCENT_OPACITY_NORMAL].toReal();
    m_accentOpacityHover = themeValues[ACCENT_OPACITY_HOVER].toReal();
    m_accentOpacityHit = themeValues[ACCENT_OPACITY_HIT].toReal();
    m_buttonOpacityNormal = themeValues[BUTTON_OPACITY_NORMAL].toReal();
    m_buttonOpacityHover = themeValues[BUTTON_OPACITY_HOVER].toReal();
    m_buttonOpacityHit = themeValues[BUTTON_OPACITY_HIT].toReal();
    m_itemOpacityDisabled = themeValues[ITEM_OPACITY_DISABLED].toReal();

    m_extra = configuration()->currentTheme().extra;
}

void ThemeApi::update()
{
    calculateDefaultButtonSize();
    initThemeValues();
    notifyAboutThemeChanged();
}

bool ThemeApi::isDark() const
{
    return configuration()->isDarkMode();
}

QColor ThemeApi::backgroundPrimaryColor() const
{
    return m_backgroundPrimaryColor;
}

QColor ThemeApi::backgroundSecondaryColor() const
{
    return m_backgroundSecondaryColor;
}

QColor ThemeApi::backgroundTertiaryColor() const
{
    return m_backgroundTertiaryColor;
}

QColor ThemeApi::backgroundQuarternaryColor() const
{
    return m_backgroundQuarternaryColor;
}

QColor ThemeApi::popupBackgroundColor() const
{
    return m_popupBackgroundColor;
}

QColor ThemeApi::textFieldColor() const
{
    return m_textFieldColor;
}

QColor ThemeApi::accentColor() const
{
    return m_accentColor;
}

QColor ThemeApi::strokeColor() const
{
    return m_strokeColor;
}

QColor ThemeApi::buttonColor() const
{
    return m_buttonColor;
}

QColor ThemeApi::fontPrimaryColor() const
{
    return m_fontPrimaryColor;
}

QColor ThemeApi::fontSecondaryColor() const
{
    return m_fontSecondaryColor;
}

QColor ThemeApi::linkColor() const
{
    return m_linkColor;
}

QColor ThemeApi::focusColor() const
{
    return m_focusColor;
}

QFont ThemeApi::bodyFont() const
{
    return m_bodyFont;
}

QFont ThemeApi::bodyBoldFont() const
{
    return m_bodyBoldFont;
}

QFont ThemeApi::largeBodyFont() const
{
    return m_largeBodyFont;
}

QFont ThemeApi::largeBodyBoldFont() const
{
    return m_largeBodyBoldFont;
}

QFont ThemeApi::tabFont() const
{
    return m_tabFont;
}

QFont ThemeApi::tabBoldFont() const
{
    return m_tabBoldFont;
}

QFont ThemeApi::headerFont() const
{
    return m_headerFont;
}

QFont ThemeApi::headerBoldFont() const
{
    return m_headerBoldFont;
}

QFont ThemeApi::titleBoldFont() const
{
    return m_titleBoldFont;
}

QFont ThemeApi::iconsFont() const
{
    return m_iconsFont;
}

QFont ThemeApi::toolbarIconsFont() const
{
    return m_toolbarIconsFont;
}

QFont ThemeApi::musicalFont() const
{
    return m_musicalFont;
}

QFont ThemeApi::musicalTextFont() const
{
    return m_musicalTextFont;
}

qreal ThemeApi::defaultButtonSize() const
{
    return m_defaultButtonSize;
}

qreal ThemeApi::borderWidth() const
{
    return m_borderWidth;
}

qreal ThemeApi::navCtrlBorderWidth() const
{
    return m_navCtrlBorderWidth;
}

qreal ThemeApi::accentOpacityNormal() const
{
    return m_accentOpacityNormal;
}

qreal ThemeApi::accentOpacityHover() const
{
    return m_accentOpacityHover;
}

qreal ThemeApi::accentOpacityHit() const
{
    return m_accentOpacityHit;
}

qreal ThemeApi::buttonOpacityNormal() const
{
    return m_buttonOpacityNormal;
}

qreal ThemeApi::buttonOpacityHover() const
{
    return m_buttonOpacityHover;
}

qreal ThemeApi::buttonOpacityHit() const
{
    return m_buttonOpacityHit;
}

qreal ThemeApi::itemOpacityDisabled() const
{
    return m_itemOpacityDisabled;
}

int ThemeApi::flickableMaxVelocity() const
{
    return configuration()->flickableMaxVelocity();
}

int ThemeApi::tooltipDelay() const
{
    return configuration()->tooltipDelay();
}

void ThemeApi::initUiFonts()
{
    setupUiFonts();

    configuration()->fontChanged().onNotify(this, [this]() {
        setupUiFonts();
        update();
    });
}

void ThemeApi::initIconsFont()
{
    setupIconsFont();

    configuration()->iconsFontChanged().onNotify(this, [this]() {
        setupIconsFont();
        update();
    });
}

void ThemeApi::initMusicalFont()
{
    setupMusicFont();

    configuration()->musicalFontChanged().onNotify(this, [this]() {
        setupMusicFont();
        update();
    });
}

void ThemeApi::initMusicalTextFont()
{
    setupMusicTextFont();

    configuration()->musicalTextFontChanged().onNotify(this, [this]() {
        setupMusicTextFont();
        update();
    });
}

void ThemeApi::setupUiFonts()
{
    const std::vector<std::pair<QFont*, FontConfig> > fonts {
        { &m_bodyFont, { QFont::Normal, FontSizeType::BODY } },
        { &m_bodyBoldFont, { QFont::DemiBold, FontSizeType::BODY } },
        { &m_largeBodyFont, { QFont::Normal, FontSizeType::BODY_LARGE } },
        { &m_largeBodyBoldFont, { QFont::DemiBold, FontSizeType::BODY_LARGE } },
        { &m_tabFont, { QFont::Normal, FontSizeType::TAB } },
        { &m_tabBoldFont, { QFont::DemiBold, FontSizeType::TAB } },
        { &m_headerFont, { QFont::Normal, FontSizeType::HEADER } },
        { &m_headerBoldFont, { QFont::DemiBold, FontSizeType::HEADER } },
        { &m_titleBoldFont, { QFont::DemiBold, FontSizeType::TITLE } },
    };

    for (const auto& [font, fontConfig] : fonts) {
        std::string family = configuration()->fontFamily();
        int size = configuration()->fontSize(fontConfig.sizeType);

        font->setPixelSize(size);
        font->setFamily(QString::fromStdString(family));
        font->setWeight(fontConfig.weight);
    }
}

void ThemeApi::setupIconsFont()
{
    QString family = QString::fromStdString(configuration()->iconsFontFamily());

    m_iconsFont.setFamily(family);
    m_iconsFont.setPixelSize(configuration()->iconsFontSize(IconSizeType::Regular));

    m_toolbarIconsFont.setFamily(family);
    m_toolbarIconsFont.setPixelSize(configuration()->iconsFontSize(IconSizeType::Toolbar));
}

void ThemeApi::setupMusicFont()
{
    m_musicalFont.setFamily(QString::fromStdString(configuration()->musicalFontFamily()));
    m_musicalFont.setPixelSize(configuration()->musicalFontSize());
}

void ThemeApi::setupMusicTextFont()
{
    m_musicalTextFont.setFamily(QString::fromStdString(configuration()->musicalTextFontFamily()));
    m_musicalTextFont.setPixelSize(configuration()->musicalTextFontSize());
}

void ThemeApi::calculateDefaultButtonSize()
{
    constexpr qreal MINIMUM_BUTTON_SIZE = 30.0;
    constexpr qreal BUTTON_PADDING = 8.0;

    QFontMetricsF bodyFontMetrics(m_bodyFont);
    QFontMetricsF iconFontMetrics(m_iconsFont);

    qreal requiredSize = std::max(bodyFontMetrics.height(), iconFontMetrics.height()) + BUTTON_PADDING;
    m_defaultButtonSize = std::max(requiredSize, MINIMUM_BUTTON_SIZE);
}

void ThemeApi::notifyAboutThemeChanged()
{
    emit themeChanged();
}

QVariantMap ThemeApi::extra() const
{
    return m_extra;
}
