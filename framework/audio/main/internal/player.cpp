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

#include <set>

#include "audio/common/rpc/rpcpacker.h"
#include "audio/common/audiosanitizer.h"

#include "log.h"

using namespace muse;
using namespace muse::async;
using namespace muse::audio;
using namespace muse::audio::rpc;

//! NOTE Playback is contextual (each window has its own), and every context can hand out
//! more than one player, so live playback is ongoing as long as any of them is running.
static std::set<const Player*>& runningPlayers()
{
    static std::set<const Player*> s_runningPlayers;
    return s_runningPlayers;
}

Player::~Player()
{
    if (runningPlayers().erase(this) != 0) {
        updateLivePlaybackOngoing();
    }
}

rpc::CtxId Player::ctxId() const
{
    return rpc::ctxId(iocContext());
}

void Player::updateLivePlaybackOngoing()
{
    ONLY_AUDIO_MAIN_THREAD;

    //! NOTE Can already be gone on shutdown
    const std::shared_ptr<IAudioDriverController>& controller = audioDriverController.get();
    if (controller) {
        controller->setLivePlaybackOngoing(!runningPlayers().empty());
    }
}

void Player::init()
{
    ONLY_AUDIO_MAIN_THREAD;

    //! NOTE Subscribe and request initial state

    {
        m_playbackStatusChanged.onReceive(this, [this](PlaybackStatus st) {
            m_playbackStatus = st;

            //! NOTE Only while running is audio actually being produced;
            //! when paused or stopped there is nothing to keep the system awake for
            if (m_playbackStatus == PlaybackStatus::Running) {
                runningPlayers().insert(this);
            } else {
                runningPlayers().erase(this);
            }

            updateLivePlaybackOngoing();
        });

        Msg msg = rpc::make_request(ctxId(), MsgCode::GetPlaybackStatus);
        channel()->send(msg, [this](const Msg& res) {
            ONLY_AUDIO_MAIN_THREAD;
            Ret ret;
            PlaybackStatus status = PlaybackStatus::Stopped;
            StreamId streamId = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(res.data, ret, status, streamId)) {
                return;
            }

            if (ret) {
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
        channel()->send(msg, [this](const Msg& res) {
            ONLY_AUDIO_MAIN_THREAD;
            Ret ret;
            secs_t pos = 0.0;
            StreamId streamId = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(res.data, ret, pos, streamId)) {
                return;
            }

            if (ret) {
                channel()->addReceiveStream(StreamName::PlaybackPositionStream, streamId, m_playbackPositionChanged);
                //! NOTE Send initial state
                m_playbackPositionChanged.send(pos);
            } else {
                LOGE() << "GetPlaybackPosition failed: " << ret.toString();
            }
        });
    }
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
