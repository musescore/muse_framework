/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore BVBA and others
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

#include "cloud/musescorecom/imusescorecomservice.h"

namespace muse::cloud {
class MuseScoreComServiceMock : public IMuseScoreComService
{
public:
    MOCK_METHOD(IAuthorizationServicePtr, authorization, (), (override));
    MOCK_METHOD(QUrl, scoreManagerUrl, (), (const, override));

    MOCK_METHOD(ProgressPtr, uploadScore,
                (DevicePtr scoreData, const QString& title, cloud::Visibility visibility, const QUrl& sourceUrl, int revisionId),
                (override));
    MOCK_METHOD(ProgressPtr, uploadAudio, (DevicePtr audioData, const QString& audioFormat, const QUrl& sourceUrl), (override));

    MOCK_METHOD(RetVal<ScoreInfo>, downloadScoreInfo, (const QUrl& sourceUrl), (override));
    MOCK_METHOD(RetVal<ScoreInfo>, downloadScoreInfo, (int scoreId), (override));
    MOCK_METHOD(async::Promise<ScoresList>, downloadScoresList, (int scoresPerBatch, int batchNumber), (override));
    MOCK_METHOD(ProgressPtr, downloadScore, (int scoreId, DevicePtr scoreData, const QString& hash, const QString& secret), (override));
};
}
