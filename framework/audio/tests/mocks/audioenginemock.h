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

#include "audio/engine/internal/iaudioengine.h"

namespace muse::audio::engine {
class AudioEngineMock : public IAudioEngine
{
public:
    MOCK_METHOD(Ret, init, (const OutputSpec&), (override));
    MOCK_METHOD(void, deinit, (), (override));

    MOCK_METHOD(RetVal<std::shared_ptr<IAudioContext> >, addAudioContext, (const AudioCtxId&), (override));
    MOCK_METHOD(std::shared_ptr<IAudioContext>, context, (const AudioCtxId&), (const, override));
    MOCK_METHOD(void, destroyContext, (const AudioCtxId&), (override));

    MOCK_METHOD(void, setOutputSpec, (const OutputSpec&), (override));
    MOCK_METHOD(OutputSpec, outputSpec, (), (const, override));
    MOCK_METHOD(async::Channel<OutputSpec>, outputSpecChanged, (), (const, override));

    MOCK_METHOD(samples_t, process, (float*, samples_t), (override));
};
}
