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

#include <map>
#include <utility>

#include "audio/common/rpc/rpcpacker.h"
#include "audio/engine/internal/enginerpccontroller.h"

#include "mocks/audiocontextmock.h"
#include "mocks/audioenginemock.h"
#include "mocks/rpcchannelmock.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::engine;
using namespace muse::audio::rpc;

namespace {
const CtxId CTX_ID = 1;
const TrackId TRACK_ID = 2;
const TrackId OTHER_TRACK_ID = 3;

class ExecOperationNow : public IExecOperation
{
public:
    void execOperation(OperationType, const Operation& func) override
    {
        func();
    }
};
}

namespace muse::audio::engine {
class Audio_EngineRpcControllerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_rpcChannel = std::make_shared<NiceMock<RpcChannelMock> >();
        m_audioEngine = std::make_shared<NiceMock<AudioEngineMock> >();
        m_audioContext = std::make_shared<NiceMock<AudioContextMock> >();

        modularity::globalIoc()->registerExport<IRpcChannel>("utests", m_rpcChannel);
        modularity::globalIoc()->registerExport<IAudioEngine>("utests", m_audioEngine);

        ON_CALL(*m_rpcChannel, onRequest(_, _, _))
        .WillByDefault(Invoke([this](CtxId ctxId, MsgCode code, RequestHandler h) {
            m_handlers[{ ctxId, code }] = h;
        }));

        ON_CALL(*m_audioEngine, addAudioContext(_))
        .WillByDefault(Return(RetVal<std::shared_ptr<IAudioContext> >::make_ok(m_audioContext)));
        ON_CALL(*m_audioEngine, context(CTX_ID)).WillByDefault(Return(m_audioContext));

        ON_CALL(*m_audioContext, trackRemoved()).WillByDefault(Return(m_trackRemoved));
        ON_CALL(*m_audioContext, signalChanges(_))
        .WillByDefault(Return(RetVal<AudioSignalChanges>::make_ok(AudioSignalChanges())));
        ON_CALL(*m_audioContext, automatedControlParamsChanges(_))
        .WillByDefault(Return(RetVal<AutomatedControlParamsChanges>::make_ok(AutomatedControlParamsChanges())));
        ON_CALL(*m_audioContext, inputProcessingProgress(_))
        .WillByDefault(Return(RetVal<InputProcessingProgress>::make_ok(InputProcessingProgress())));

        m_controller = std::make_shared<EngineRpcController>(&m_execOperation);
        m_controller->init();
    }

    void TearDown() override
    {
        m_controller->deinit();
        m_controller.reset();

        modularity::globalIoc()->unregister<IRpcChannel>("utests");
        modularity::globalIoc()->unregister<IAudioEngine>("utests");
    }

    Msg request(CtxId ctxId, MsgCode code, const ByteArray& data = ByteArray())
    {
        auto it = m_handlers.find({ ctxId, code });
        if (it == m_handlers.end() || !it->second) {
            ADD_FAILURE() << "no handler: " << rpc::to_string(code);
            return Msg();
        }

        return it->second(make_request(ctxId, code, data));
    }

    //! NOTE Returns the id of the stream added by the engine for the response
    StreamId requestStream(MsgCode code, const ByteArray& data = ByteArray())
    {
        StreamId streamId = 0;
        EXPECT_CALL(*m_rpcChannel, addStream(_))
        .WillOnce(Invoke([&streamId](std::shared_ptr<IRpcStream> s) {
            //! NOTE The stream must belong to the context to be removed with it
            EXPECT_EQ(s->ctxId(), CTX_ID);
            streamId = s->streamId();
        }));

        request(CTX_ID, code, data);
        ::testing::Mock::VerifyAndClearExpectations(m_rpcChannel.get());

        return streamId;
    }

    ExecOperationNow m_execOperation;

    std::shared_ptr<RpcChannelMock> m_rpcChannel;
    std::shared_ptr<AudioEngineMock> m_audioEngine;
    std::shared_ptr<AudioContextMock> m_audioContext;
    std::shared_ptr<EngineRpcController> m_controller;

    std::map<std::pair<CtxId, MsgCode>, RequestHandler> m_handlers;
    async::Channel<TrackId> m_trackRemoved;
};

TEST_F(Audio_EngineRpcControllerTests, ContextDeinit_CancelsAllRequestsOfContext)
{
    // [GIVEN] The context is inited
    request(GLOBAL_CTX_ID, MsgCode::ContextInit, RpcPacker::pack(CTX_ID));

    // [GIVEN] The player has requested the status and the position
    requestStream(MsgCode::GetPlaybackStatus);
    requestStream(MsgCode::GetPlaybackPosition);

    // [THEN] The pending responses and the streams of the context are removed by the channel
    EXPECT_CALL(*m_rpcChannel, cancelAllRequests(CTX_ID));
    EXPECT_CALL(*m_rpcChannel, removeStream(_)).Times(0);

    // [WHEN] Deinit the context
    request(GLOBAL_CTX_ID, MsgCode::ContextDeinit, RpcPacker::pack(CTX_ID));
}

TEST_F(Audio_EngineRpcControllerTests, ContextDeinit_ForgetsTrackStreams)
{
    // [GIVEN] The context is inited and the stream of the track is requested
    request(GLOBAL_CTX_ID, MsgCode::ContextInit, RpcPacker::pack(CTX_ID));
    requestStream(MsgCode::GetSignalChanges, RpcPacker::pack(TRACK_ID));

    // [GIVEN] The context is deinited and inited again
    request(GLOBAL_CTX_ID, MsgCode::ContextDeinit, RpcPacker::pack(CTX_ID));
    request(GLOBAL_CTX_ID, MsgCode::ContextInit, RpcPacker::pack(CTX_ID));

    // [THEN] The stream of the previous init is not removed again
    EXPECT_CALL(*m_rpcChannel, removeStream(_)).Times(0);

    // [WHEN] The track is removed
    m_trackRemoved.send(TRACK_ID);
}

TEST_F(Audio_EngineRpcControllerTests, TrackRemoved_RemovesTrackStreams)
{
    // [GIVEN] The context is inited
    request(GLOBAL_CTX_ID, MsgCode::ContextInit, RpcPacker::pack(CTX_ID));

    // [GIVEN] The streams of two tracks are requested
    const StreamId signalStreamId = requestStream(MsgCode::GetSignalChanges, RpcPacker::pack(TRACK_ID));
    const StreamId automationStreamId = requestStream(MsgCode::GetAutomatedControlParamsChanges, RpcPacker::pack(TRACK_ID));
    const StreamId progressStreamId = requestStream(MsgCode::GetInputProcessingProgress, RpcPacker::pack(TRACK_ID));
    const StreamId otherTrackStreamId = requestStream(MsgCode::GetSignalChanges, RpcPacker::pack(OTHER_TRACK_ID));

    // [THEN] Only the streams of the removed track are removed
    EXPECT_CALL(*m_rpcChannel, removeStream(signalStreamId));
    EXPECT_CALL(*m_rpcChannel, removeStream(automationStreamId));
    EXPECT_CALL(*m_rpcChannel, removeStream(progressStreamId));
    EXPECT_CALL(*m_rpcChannel, removeStream(otherTrackStreamId)).Times(0);

    // [WHEN] The track is removed
    m_trackRemoved.send(TRACK_ID);
    ::testing::Mock::VerifyAndClearExpectations(m_rpcChannel.get());

    // [THEN] The streams of the other track are removed on the context deinit by the channel with all streams of the context
    EXPECT_CALL(*m_rpcChannel, cancelAllRequests(CTX_ID));
    EXPECT_CALL(*m_rpcChannel, removeStream(_)).Times(0);

    // [WHEN] Deinit the context
    request(GLOBAL_CTX_ID, MsgCode::ContextDeinit, RpcPacker::pack(CTX_ID));
}
}
