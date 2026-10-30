/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited and others
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
#include "player.h"

#include "audio/common/rpc/rpcpacker.h"
#include "audio/common/audiosanitizer.h"

#include "log.h"

using namespace muse;
using namespace muse::async;
using namespace muse::audio;
using namespace muse::audio::rpc;

rpc::CtxId Player::ctxId() const
{
    return rpc::ctxId(iocContext());
}

void Player::init()
{
    ONLY_AUDIO_MAIN_THREAD;

    //! NOTE Subscribe and request initial state

    {
        m_playbackStatusChanged.onReceive(this, [this](PlaybackStatus st) {
            m_playbackStatus = st;
        });

        Msg msg = rpc::make_request(ctxId(), MsgCode::GetPlaybackStatus);
        channel()->send(msg, [this, alive = std::weak_ptr<bool>(m_alive)](const Msg& res) {
            ONLY_AUDIO_MAIN_THREAD;
            if (alive.expired()) {
                return;
            }

            Ret ret;
            PlaybackStatus status = PlaybackStatus::Stopped;
            StreamId streamId = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(res.data, ret, status, streamId)) {
                return;
            }

            if (ret) {
                m_playbackStatusStreamId = streamId;
                channel()->addReceiveStream(StreamName::PlaybackStatusStream, streamId, m_playbackStatusChanged);
                //! NOTE Send initial state
                m_playbackStatusChanged.send(status);
            } else {
                LOGE() << "GetPlaybackStatus failed: " << ret.toString();
            }
        });
    }

    {
        m_playbackPositionChanged.onReceive(this, [this](const secs_t newPos) {
            m_playbackPosition = newPos;
        });

        Msg msg = rpc::make_request(ctxId(), MsgCode::GetPlaybackPosition);
        channel()->send(msg, [this, alive = std::weak_ptr<bool>(m_alive)](const Msg& res) {
            ONLY_AUDIO_MAIN_THREAD;
            if (alive.expired()) {
                return;
            }

            Ret ret;
            secs_t pos = 0.0;
            StreamId streamId = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(res.data, ret, pos, streamId)) {
                return;
            }

            if (ret) {
                m_playbackPositionStreamId = streamId;
                channel()->addReceiveStream(StreamName::PlaybackPositionStream, streamId, m_playbackPositionChanged);
                //! NOTE Send initial state
                m_playbackPositionChanged.send(pos);
            } else {
                LOGE() << "GetPlaybackPosition failed: " << ret.toString();
            }
        });
    }
}

void Player::deinit()
{
    ONLY_AUDIO_MAIN_THREAD;

    m_alive.reset();

    if (m_playbackStatusStreamId != 0) {
        channel()->removeStream(m_playbackStatusStreamId);
        m_playbackStatusStreamId = 0;
    }

    if (m_playbackPositionStreamId != 0) {
        channel()->removeStream(m_playbackPositionStreamId);
        m_playbackPositionStreamId = 0;
    }

    m_playbackStatusChanged.disconnect(this);
    m_playbackPositionChanged.disconnect(this);
}

async::Promise<Ret> Player::prepareToPlay()
{
    ONLY_AUDIO_MAIN_THREAD;
    return async::make_promise<Ret>([this](auto resolve, auto) {
        ONLY_AUDIO_MAIN_THREAD;
        Msg msg = rpc::make_request(ctxId(), MsgCode::PrepareToPlay);
        channel()->send(msg, [resolve](const Msg& res) {
            ONLY_AUDIO_MAIN_THREAD;
            Ret ret;
            IF_ASSERT_FAILED(RpcPacker::unpack(res.data, ret)) {
                (void)resolve(make_ret(Ret::Code::UnknownError));
                return;
            }

            (void)resolve(ret);
        });
        return Promise<Ret>::dummy_result();
    }, PromiseType::AsyncByBody);
}

void Player::play(const secs_t delay)
{
    ONLY_AUDIO_MAIN_THREAD;
    Msg msg = rpc::make_request(ctxId(), MsgCode::Play, RpcPacker::pack(delay));
    channel()->send(msg);
}

void Player::seek(const secs_t newPosition, const bool flushSound)
{
    ONLY_AUDIO_MAIN_THREAD;

    if (m_playbackPosition == newPosition) {
        return;
    }

    Msg msg = rpc::make_request(ctxId(), MsgCode::Seek, RpcPacker::pack(newPosition, flushSound));
    channel()->send(msg);
}

void Player::stop()
{
    ONLY_AUDIO_MAIN_THREAD;
    Msg msg = rpc::make_request(ctxId(), MsgCode::Stop);
    channel()->send(msg);
}

void Player::pause()
{
    ONLY_AUDIO_MAIN_THREAD;
    Msg msg = rpc::make_request(ctxId(), MsgCode::Pause);
    channel()->send(msg);
}

void Player::resume(const secs_t delay)
{
    ONLY_AUDIO_MAIN_THREAD;
    Msg msg = rpc::make_request(ctxId(), MsgCode::Resume, RpcPacker::pack(delay));
    channel()->send(msg);
}

void Player::setDuration(const secs_t duration)
{
    ONLY_AUDIO_MAIN_THREAD;
    Msg msg = rpc::make_request(ctxId(), MsgCode::SetDuration, RpcPacker::pack(duration));
    channel()->send(msg);
}

async::Promise<bool> Player::setLoop(const secs_t from, const secs_t to)
{
    ONLY_AUDIO_MAIN_THREAD;
    return async::make_promise<bool>([this, from, to](auto resolve, auto reject) {
        ONLY_AUDIO_MAIN_THREAD;
        Msg msg = rpc::make_request(ctxId(), MsgCode::SetLoop, RpcPacker::pack(from, to));
        channel()->send(msg, [resolve, reject](const Msg& res) {
            ONLY_AUDIO_MAIN_THREAD;
            Ret ret;
            IF_ASSERT_FAILED(RpcPacker::unpack(res.data, ret)) {
                return;
            }

            if (ret) {
                (void)resolve(true);
            } else {
                (void)reject(ret.code(), ret.text());
            }
        });
        return Promise<bool>::dummy_result();
    }, PromiseType::AsyncByBody);
}

void Player::resetLoop()
{
    ONLY_AUDIO_MAIN_THREAD;
    Msg msg = rpc::make_request(ctxId(), MsgCode::ResetLoop);
    channel()->send(msg);
}

secs_t Player::playbackPosition() const
{
    ONLY_AUDIO_MAIN_THREAD;
    return m_playbackPosition;
}

async::Channel<secs_t> Player::playbackPositionChanged() const
{
    ONLY_AUDIO_MAIN_THREAD;
    return m_playbackPositionChanged;
}

PlaybackStatus Player::playbackStatus() const
{
    ONLY_AUDIO_MAIN_THREAD;
    return m_playbackStatus;
}

async::Channel<PlaybackStatus> Player::playbackStatusChanged() const
{
    ONLY_AUDIO_MAIN_THREAD;
    return m_playbackStatusChanged;
}
