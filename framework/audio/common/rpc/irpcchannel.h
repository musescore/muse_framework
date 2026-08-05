/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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

#include <functional>
#include <tuple>

#include "global/modularity/imoduleinterface.h"
#include "global/modularity/ioc.h"

#include "global/types/bytearray.h"
#include "global/async/channel.h"
#include "global/async/asyncable.h"

#include "rpcpacker.h"

namespace muse::audio::rpc {
using CallId = uint64_t;
using CtxId = uint16_t;
constexpr CtxId GLOBAL_CTX_ID = 0;

inline CtxId ctxId(const muse::modularity::ContextPtr& ctx)
{
    IF_ASSERT_FAILED(ctx) {
        return GLOBAL_CTX_ID;
    }
    IF_ASSERT_FAILED(ctx->id > 0) {
        return GLOBAL_CTX_ID;
    }
    return static_cast<CtxId>(ctx->id);
}

enum class MsgCode {
    Undefined = 0,

    // Init / Deinit
    EngineInit,
    EngineRunning, // notification
    EngineDeinit,

    ContextInit,
    ContextDeinit,

    // Config
    EngineConfigChanged,

    // AudioEngine
    SetOutputSpec,

    // Resources
    GetAvailableInputResources,
    GetAvailableSoundPresets,
    GetAvailableOutputResources,

    // Tracks
    RemoveTrack,
    RemoveAllTracks,
    GetTrackIdList,
    GetTrackName,
    AddTrackWithPlaybackData,
    AddTrackWithIODevice,
    AddAuxTrack,
    // notification
    TrackAdded,
    TrackRemoved,

    // Params
    GetTrackParams,
    SetSourceParams,
    SetControlParams,
    SetFxChainParams,
    SetAuxSendsParams,
    // notification
    SourceParamsChanged,
    FxChainParamsChanged,

    // Input processing
    GetInputProcessingProgress,
    ProcessInput,

    // Clear
    ClearCache,
    ClearSources,
    ClearMasterOutputParams,
    ClearAllFx,

    // Play
    PrepareToPlay,
    Play,
    Seek,
    Stop,
    Pause,
    Resume,
    SetDuration,
    SetLoop,
    ResetLoop,
    GetPlaybackStatus,
    GetPlaybackPosition,

    // Signals
    GetSignalChanges,
    GetAutomatedControlParamsChanges,

    // Export
    SaveSoundTrack,
    AbortSavingAllSoundTracks,
    GetSaveSoundTrackProgress,

    // SoundFont
    LoadSoundFonts,
    AddSoundFont,
    AddSoundFontData,

    // Transport
    TransportEventReceived,

    // Streams
    FirstStream = 100,
    PlaybackDataMainStream,
    PlaybackDataOffStream,
    AudioSignalStream,
    AudioMasterSignalStream,
    AutomatedControlParamsStream,
    PlaybackStatusStream,
    PlaybackPositionStream,
    InputProcessingProgressStream,
    SaveSoundTrackProgressStream
};

inline std::string to_string(MsgCode m)
{
    switch (m) {
    case MsgCode::Undefined: return "Undefined";

    // Init / Deinit
    case MsgCode::EngineRunning: return "EngineRunning";
    case MsgCode::EngineInit: return "EngineInit";
    case MsgCode::EngineDeinit: return "EngineDeinit";

    case MsgCode::ContextInit: return "ContextInit";
    case MsgCode::ContextDeinit: return "ContextDeinit";

    // Config
    case MsgCode::EngineConfigChanged: return "EngineConfigChanged";

    // AudioEngine
    case MsgCode::SetOutputSpec: return "SetOutputSpec";

    // Resources
    case MsgCode::GetAvailableInputResources: return "GetAvailableInputResources";
    case MsgCode::GetAvailableSoundPresets: return "GetAvailableSoundPresets";
    case MsgCode::GetAvailableOutputResources: return "GetAvailableOutputResources";

    // Tracks
    case MsgCode::RemoveTrack: return "RemoveTrack";
    case MsgCode::RemoveAllTracks: return "RemoveAllTracks";
    case MsgCode::GetTrackIdList: return "GetTrackIdList";
    case MsgCode::GetTrackName: return "GetTrackName";
    case MsgCode::AddTrackWithPlaybackData: return "AddTrackWithPlaybackData";
    case MsgCode::AddTrackWithIODevice: return "AddTrackWithIODevice";
    case MsgCode::AddAuxTrack: return "AddAuxTrack";
    case MsgCode::TrackAdded: return "TrackAdded";
    case MsgCode::TrackRemoved: return "TrackRemoved";

    // Params
    case MsgCode::GetTrackParams: return "GetTrackParams";
    case MsgCode::SetSourceParams: return "SetSourceParams";
    case MsgCode::SetControlParams: return "SetControlParams";
    case MsgCode::SetFxChainParams: return "SetFxChainParams";
    case MsgCode::SetAuxSendsParams: return "SetAuxSendsParams";
    // notification
    case MsgCode::SourceParamsChanged: return "SourceParamsChanged";
    case MsgCode::FxChainParamsChanged: return "FxChainParamsChanged";

    // Input processing
    case MsgCode::GetInputProcessingProgress: return "GetInputProcessingProgress";
    case MsgCode::ProcessInput: return "ProcessInput";

    // Clear
    case MsgCode::ClearCache: return "ClearCache";
    case MsgCode::ClearSources: return "ClearSources";
    case MsgCode::ClearMasterOutputParams: return "ClearMasterOutputParams";
    case MsgCode::ClearAllFx: return "ClearAllFx";

    // Play
    case MsgCode::PrepareToPlay: return "PrepareToPlay";
    case MsgCode::Play: return "Play";
    case MsgCode::Seek: return "Seek";
    case MsgCode::Stop: return "Stop";
    case MsgCode::Pause: return "Pause";
    case MsgCode::Resume: return "Resume";
    case MsgCode::SetDuration: return "SetDuration";
    case MsgCode::SetLoop: return "SetLoop";
    case MsgCode::ResetLoop: return "ResetLoop";
    case MsgCode::GetPlaybackStatus: return "GetPlaybackStatus";
    case MsgCode::GetPlaybackPosition: return "GetPlaybackPosition";

    // Signals
    case MsgCode::GetSignalChanges: return "GetSignalChanges";
    case MsgCode::GetAutomatedControlParamsChanges: return "GetAutomatedControlParamsChanges";

    case MsgCode::SaveSoundTrack: return "SaveSoundTrack";
    case MsgCode::AbortSavingAllSoundTracks: return "AbortSavingAllSoundTracks";
    case MsgCode::GetSaveSoundTrackProgress: return "GetSaveSoundTrackProgress";

    // SoundFont
    case MsgCode::LoadSoundFonts: return "LoadSoundFonts";
    case MsgCode::AddSoundFont: return "AddSoundFont";
    case MsgCode::AddSoundFontData: return "AddSoundFontData";

    // Transport
    case MsgCode::TransportEventReceived: return "TransportEventReceived";

    // Streams
    case MsgCode::FirstStream: return "FirstStream";
    case MsgCode::PlaybackDataMainStream: return "PlaybackDataMainStream";
    case MsgCode::PlaybackDataOffStream: return "PlaybackDataOffStream";
    case MsgCode::AudioSignalStream: return "AudioSignalStream";
    case MsgCode::AudioMasterSignalStream: return "AudioMasterSignalStream";
    case MsgCode::AutomatedControlParamsStream: return "AutomatedControlParamsStream";
    case MsgCode::PlaybackStatusStream: return "PlaybackStatusStream";
    case MsgCode::PlaybackPositionStream: return "PlaybackPositionStream";
    case MsgCode::InputProcessingProgressStream: return "InputProcessingProgressStream";
    case MsgCode::SaveSoundTrackProgressStream: return "SaveSoundTrackProgressStream";
    }

    assert(false && "unknown enum value");
    return std::to_string(static_cast<int>(m));
}

enum class MsgType {
    Undefined = 0,
    Notification,
    Request,
    Response,
    ResponseDelayed,
    Stream
};

inline std::string to_string(MsgType t)
{
    switch (t) {
    case MsgType::Undefined: return "Undefined";
    case MsgType::Notification: return "Notification";
    case MsgType::Request: return "Request";
    case MsgType::Response: return "Response";
    case MsgType::ResponseDelayed: return "ResponseDelayed";
    case MsgType::Stream: return "Stream";
    }

    assert(false && "unknown enum value");
    return std::to_string(static_cast<int>(t));
}

struct Msg {
    CallId callId = 0;
    CtxId ctxId = 0;   // 0 - global, >0 - contextual
    MsgCode code = MsgCode::Undefined;
    MsgType type = MsgType::Undefined;
    ByteArray data;
};

struct MsgKey {
    CtxId ctxId = 0;
    MsgCode code = MsgCode::Undefined;

    bool operator==(const MsgKey& other) const
    {
        return ctxId == other.ctxId && code == other.code;
    }

    bool operator<(const MsgKey& other) const
    {
        return std::tie(ctxId, code) < std::tie(other.ctxId, other.code);
    }
};

using StreamId = CallId;
using StreamName = MsgCode;
using StreamMsg = Msg;

inline StreamId& last_stream_id()
{
    static StreamId lastStreamId = 0;
    return lastStreamId;
}

inline void set_last_stream_id(StreamId id)
{
    last_stream_id() = id;
}

inline StreamId new_stream_id()
{
    ++last_stream_id();
    return last_stream_id();
}

using StreamHandler = std::function<void (const Msg& msg)>;

enum class StreamType {
    Undefined = 0,
    Send,
    Receive
};

struct IRpcStream {
    virtual ~IRpcStream() = default;

    virtual void setCtxId(CtxId ctxId) = 0;
    virtual CtxId ctxId() const = 0;
    virtual StreamId streamId() const = 0;
    virtual StreamType type() const = 0;
    virtual void init() = 0;
    virtual bool inited() const = 0;
};

using RpcStreamExec = std::function<void (const std::function<void ()>&)>;

class IStreamRpcChannel;
template<typename ... Types>
class RpcStream : public IRpcStream, public async::Asyncable
{
public:

    RpcStream(IStreamRpcChannel* rpc, StreamName name, StreamId id, StreamType type,
              const async::Channel<Types...>& ch,
              const RpcStreamExec& exec)
        : m_rpc(rpc), m_name(name), m_streamId(id), m_type(type), m_ch(ch), m_exec(exec) {}

    ~RpcStream()
    {
        deinit();
    }

    void setCtxId(CtxId ctxId) override { m_ctxId = ctxId; }
    CtxId ctxId() const override { return m_ctxId; }
    StreamName name() const { return m_name; }
    StreamId streamId() const override { return m_streamId; }
    StreamType type() const override { return m_type; }
    void init() override;
    bool inited() const override { return m_inited; }
    void deinit();

private:
    IStreamRpcChannel* m_rpc = nullptr;
    CtxId m_ctxId = 0;
    StreamName m_name = StreamName::Undefined;
    StreamId m_streamId = 0;
    StreamType m_type = StreamType::Undefined;
    async::Channel<Types...> m_ch;
    RpcStreamExec m_exec = nullptr;
    bool m_inited = false;
};

class IStreamRpcChannel
{
public:
    virtual ~IStreamRpcChannel() = default;

    virtual void addStream(std::shared_ptr<IRpcStream> s) = 0;
    virtual void removeStream(StreamId id) = 0;
    virtual void sendStream(const StreamMsg& msg) = 0;
    virtual void onStream(StreamId id, StreamHandler h) = 0;
};

using RequestHandler = std::function<Msg (const Msg& msg)>;
using ResponseHandler = std::function<void (const Msg& msg)>;
using NotificationHandler = std::function<void (const Msg& msg)>;

class IRpcChannel : MODULE_GLOBAL_INTERFACE, public IStreamRpcChannel
{
    INTERFACE_ID(IRpcChannel)
public:
    virtual ~IRpcChannel() = default;

    virtual void setupOnMain() = 0;
    virtual void setupOnEngine() = 0;

    virtual void process() = 0;

    virtual void send(const Msg& msg, const ResponseHandler& onResponse = nullptr) = 0;
    virtual void onRequest(CtxId ctxId, MsgCode code, RequestHandler h) = 0;
    virtual void onNotification(CtxId ctxId, MsgCode code, NotificationHandler h) = 0;

    // stream (async/channel)
    template<typename ... Types>
    StreamId addSendStream(StreamName name, const async::Channel<Types...>& ch)
    {
        StreamId id = new_stream_id();
        auto s = new RpcStream<Types...>(this, name, id, StreamType::Send, ch, nullptr);
        addStream(std::shared_ptr<IRpcStream>(s));
        return id;
    }

    template<typename ... Types>
    void addReceiveStream(StreamName name, rpc::StreamId id, const async::Channel<Types...>& ch, const RpcStreamExec& exec = nullptr)
    {
        auto s = new RpcStream<Types...>(this, name, id, StreamType::Receive, ch, exec);
        addStream(std::shared_ptr<IRpcStream>(s));
    }
};

inline StreamMsg make_stream_msg(CtxId ctxId, StreamId id, StreamName name, const ByteArray& data = ByteArray())
{
    return StreamMsg{ id, ctxId, name, MsgType::Stream, data };
}

template<typename ... Types>
void RpcStream<Types...>::init()
{
    if (m_inited) {
        return;
    }

    switch (m_type) {
    case StreamType::Send: {
        m_ch.onReceive(this, [this](const Types... args) {
                ByteArray data = RpcPacker::pack(args ...);
                m_rpc->sendStream(make_stream_msg(m_ctxId, m_streamId, m_name, data));
            });
    } break;
    case StreamType::Receive: {
        m_rpc->onStream(m_streamId, [this](const StreamMsg& msg) {
                std::function<void()> func = [this, msg]() {
                    std::tuple<Types...> values;
                    bool success = std::apply([msg](auto&... args) {
                        return RpcPacker::unpack(msg.data, args ...);
                    }, values);

                    if (success) {
                        std::apply([this](const auto&... args) {
                            m_ch.send(args ...);
                        }, values);
                    }
                };
                if (m_exec) {
                    m_exec(func);
                } else {
                    func();
                }
            });
    } break;
    case StreamType::Undefined: {
    } break;
    }

    m_inited = true;
}

template<typename ... Types>
void RpcStream<Types...>::deinit()
{
    if (!m_inited) {
        return;
    }

    switch (m_type) {
    case StreamType::Send: {
        m_ch.disconnect(this);
    } break;
    case StreamType::Receive: {
        m_rpc->onStream(m_streamId, nullptr);
    } break;
    case StreamType::Undefined: {
    } break;
    }

    m_inited = false;
}

// msgs
inline CallId new_call_id()
{
    static CallId lastId = 0;
    ++lastId;
    return lastId;
}

inline Msg make_request(CtxId ctxId, MsgCode code, const ByteArray& data = ByteArray())
{
    Msg msg;
    msg.callId = new_call_id();
    msg.ctxId = ctxId;
    msg.code = code;
    msg.type = MsgType::Request;
    msg.data = data;
    return msg;
}

inline Msg make_response(const Msg& req, const ByteArray& data = ByteArray())
{
    Msg msg;
    msg.ctxId = req.ctxId;
    msg.callId = req.callId;
    msg.code = req.code;
    msg.type = MsgType::Response;
    msg.data = data;
    return msg;
}

//! NOTE Service message means that the response will be sent later
inline Msg make_response_delayed(const Msg& req)
{
    Msg r = make_response(req);
    r.type = MsgType::ResponseDelayed;
    return r;
}

inline Msg make_response_ret(const Msg& req, const Ret& ret)
{
    return make_response(req, RpcPacker::pack(ret));
}

template<typename T>
inline Msg make_response_ret(const Msg& req, const RetVal<T>& ret)
{
    return make_response(req, RpcPacker::pack(ret));
}

inline Msg make_notification(CtxId ctxId, MsgCode code, const ByteArray& data = ByteArray())
{
    Msg msg;
    msg.callId = new_call_id();
    msg.ctxId = ctxId;
    msg.code = code;
    msg.type = MsgType::Notification;
    msg.data = data;
    return msg;
}
}
