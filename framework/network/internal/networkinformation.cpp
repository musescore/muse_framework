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
#include "networkinformation.h"

#include <QNetworkInformation>

using namespace muse::network;

bool NetworkInformation::isMetered() const
{
    QNetworkInformation* info = QNetworkInformation::instance();
    if (!info) {
        if (!QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Features(QNetworkInformation::Feature::Metered))) {
            //! NOTE: No backend able to report metering on this platform - assume unmetered
            return false;
        }

        info = QNetworkInformation::instance();
    }

    return info && info->isMetered();
}
