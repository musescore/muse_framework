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
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>

#include "async/processevents.h"

#include "audio/common/rpc/rpcpacker.h"
#include "audio/main/internal/playback.h"

#include "mocks/rpcchannelmock.h"
#include "mocks/startaudiocontrollermock.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::rpc;

namespace {
const StreamId STATUS_STREAM_ID = 11;
const StreamId POSITION_STREAM_ID = 12;

MATCHER_P(StreamIdIs, id, "")
{
    return arg && arg->streamId() == id;
}
}

namespace muse::audio {
class Audio_PlaybackTests : public ::testing::Test
{
protected:
    struct Request {
        Msg msg;
        ResponseHandler onResponse;
    };

    void SetUp() override
    {
        m_rpcChannel = std::make_shared<NiceMock<RpcChannelMock> >();
        m_startAudioController = std::make_shared<NiceMock<StartAudioControllerMock> >();

        modularity::globalIoc()->registerExport<IRpcChannel>("utests", m_rpcChannel);
        modularity::globalIoc()->registerExport<IStartAudioController>("utests", m_startAudioController);

        ON_CALL(*m_startAudioController, isAudioStarted()).WillByDefault(Return(true));

        ON_CALL(*m_rpcChannel, send(_, _))
        .WillByDefault(Invoke([this](const Msg& msg, const ResponseHandler& onResponse) {
            m_requests.push_back({ msg, onResponse });
        }));

        m_playback = std::make_shared<Playback>(m_ctx);
    }

    void TearDown() override
    {
        m_playback.reset();

        modularity::globalIoc()->unregister<IRpcChannel>("utests");
        modularity::globalIoc()->unregister<IStartAudioController>("utests");
    }

    size_t requestCount(MsgCode code) const
    {
        size_t count = 0;
        for (const Request& r : m_requests) {
            if (r.msg.code == code) {
                ++count;
            }
        }
        return count;
    }

    //! NOTE Responds to the first pending request with the given code
    void respond(MsgCode code, const ByteArray& data)
    {
        for (auto it = m_requests.begin(); it != m_requests.end(); ++it) {
            if (it->msg.code == code) {
                Request r = *it;
                m_requests.erase(it);
                ASSERT_TRUE(r.onResponse);
                r.onResponse(make_response(r.msg, data));
                return;
            }
        }

        FAIL() << "no pending request: " << rpc::to_string(code);
    }

    void initContext()
    {
        m_playback->init();
        async::processMessages();

        respond(MsgCode::ContextInit, RpcPacker::pack(make_ok()));
        ASSERT_TRUE(m_playback->isInited());
    }

    void respondPlayerStreams()
    {
        respond(MsgCode::GetPlaybackStatus, RpcPacker::pack(make_ok(), PlaybackStatus::Running, STATUS_STREAM_ID));
        respond(MsgCode::GetPlaybackPosition, RpcPacker::pack(make_ok(), secs_t(2.0), POSITION_STREAM_ID));
    }

    modularity::ContextPtr m_ctx = std::make_shared<modularity::Context>(1);

    std::shared_ptr<RpcChannelMock> m_rpcChannel;
    std::shared_ptr<StartAudioControllerMock> m_startAudioController;
    std::shared_ptr<Playback> m_playback;

    std::vector<Request> m_requests;
};

TEST_F(Audio_PlaybackTests, Player_IsCreatedOncePerContext)
{
    // [GIVEN] The context is inited
    initContext();

    // [WHEN] Get the player twice
    IPlayerPtr player1 = m_playback->player();
    IPlayerPtr player2 = m_playback->player();

    // [THEN] The same player is returned and it requests the state only once
    ASSERT_TRUE(player1);
    EXPECT_EQ(player1, player2);
    EXPECT_EQ(requestCount(MsgCode::GetPlaybackStatus), 1);
    EXPECT_EQ(requestCount(MsgCode::GetPlaybackPosition), 1);
}

TEST_F(Audio_PlaybackTests, Player_SubscribesToStatusAndPosition)
{
    // [GIVEN] The context is inited
    initContext();

    // [THEN] The streams of the status and the position are added
    EXPECT_CALL(*m_rpcChannel, addStream(StreamIdIs(STATUS_STREAM_ID)));
    EXPECT_CALL(*m_rpcChannel, addStream(StreamIdIs(POSITION_STREAM_ID)));

    // [WHEN] Get the player and the engine responds
    IPlayerPtr player = m_playback->player();
    respondPlayerStreams();

    // [THEN] The player has the initial state
    EXPECT_EQ(player->playbackStatus(), PlaybackStatus::Running);
    EXPECT_EQ(player->playbackPosition(), secs_t(2.0));
}

TEST_F(Audio_PlaybackTests, Deinit_RemovesPlayerStreams)
{
    // [GIVEN] The player is subscribed to the streams
    initContext();
    IPlayerPtr player = m_playback->player();
    respondPlayerStreams();

    // [THEN] The streams are removed
    EXPECT_CALL(*m_rpcChannel, removeStream(STATUS_STREAM_ID));
    EXPECT_CALL(*m_rpcChannel, removeStream(POSITION_STREAM_ID));

    // [WHEN] Deinit the playback
    m_playback->deinit();
    EXPECT_FALSE(m_playback->isInited());

    // [WHEN] Init it again
    initContext();

    // [THEN] A new player is created
    IPlayerPtr newPlayer = m_playback->player();
    ASSERT_TRUE(newPlayer);
    EXPECT_NE(newPlayer, player);
}

TEST_F(Audio_PlaybackTests, Deinit_LateResponsesAreIgnored)
{
    // [GIVEN] The player has not received the responses yet
    initContext();
    IPlayerPtr player = m_playback->player();

    // [WHEN] Deinit the playback
    m_playback->deinit();

    // [THEN] The streams are not added by the late responses
    EXPECT_CALL(*m_rpcChannel, addStream(_)).Times(0);

    // [WHEN] The engine responds after deinit
    respondPlayerStreams();

    // [THEN] The player state is not changed
    EXPECT_EQ(player->playbackStatus(), PlaybackStatus::Stopped);
}
}
