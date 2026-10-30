/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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

#include <gmock/gmock.h>

#include "audio/main/istartaudiocontroller.h"

namespace muse::audio {
class StartAudioControllerMock : public IStartAudioController
{
public:
    MOCK_METHOD(void, startAudioProcessing, (const IApplication::RunMode&), (override));
    MOCK_METHOD(void, stopAudioProcessing, (), (override));

    MOCK_METHOD(bool, isAudioStarted, (), (const, override));
    MOCK_METHOD(async::Channel<bool>, isAudioStartedChanged, (), (const, override));
};
}
