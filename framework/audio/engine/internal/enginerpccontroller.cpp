/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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
#include "enginerpccontroller.h"

#include "audio/common/audiosanitizer.h"
#include "audio/common/rpc/rpcpacker.h"
#include "audio/common/audioerrors.h"

#include "log.h"

//#define CHECK_METHODS_DURATION

#ifdef CHECK_METHODS_DURATION
#include <chrono>
#define BEGIN_METHOD_DURATION \
    auto _start_clock = std::chrono::high_resolution_clock::now();
#define END_METHOD_DURATION(method) \
    auto _end_clock = std::chrono::high_resolution_clock::now(); \
    auto _duration_us = std::chrono::duration_cast<std::chrono::microseconds>(_end_clock - _start_clock); \
    LOGDA() << rpc::to_string(method) << " duration: " << (_duration_us.count() / 1000.0) << " ms";
#else
#define BEGIN_METHOD_DURATION
#define END_METHOD_DURATION(method)
#endif

using namespace muse::audio::engine;
using namespace muse::audio::rpc;

EngineRpcController::EngineRpcController(IExecOperation* execOperation)
    : m_execOperation(execOperation)
{
    IF_ASSERT_FAILED(m_execOperation) {
        return;
    }
}

std::shared_ptr<IAudioContext> EngineRpcController::audioContext(rpc::CtxId ctxId) const
{
    IF_ASSERT_FAILED(ctxId > 0) {
        return nullptr;
    }
    auto audioContext = audioEngine()->context(static_cast<AudioCtxId>(ctxId));
    IF_ASSERT_FAILED(audioContext) {
        return nullptr;
    }
    return audioContext;
}

void EngineRpcController::init()
{
    ONLY_AUDIO_RPC_THREAD;

    // AudioEngine
    onLongRequest(GLOBAL_CTX_ID, MsgCode::SetOutputSpec, [this](const Msg& msg) {
        OutputSpec spec;
        IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, spec)) {
            return make_response_ret(msg, make_ret(Err::InvalidRpcData));
        }
        audioEngine()->setOutputSpec(spec);

        return make_response_ret(msg, make_ok());
    });

    // Soundfont
    onLongRequest(GLOBAL_CTX_ID, MsgCode::LoadSoundFonts, [this](const Msg& msg) {
        std::vector<synth::SoundFontUri> uris;
        IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, uris)) {
            return make_response_ret(msg, make_ret(Err::InvalidRpcData));
        }

        soundFontRepository()->loadSoundFonts(uris);

        return make_response_ret(msg, make_ok());
    });

    onLongRequest(GLOBAL_CTX_ID, MsgCode::AddSoundFont, [this](const Msg& msg) {
        synth::SoundFontUri uri;
        IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, uri)) {
            return make_response_ret(msg, make_ret(Err::InvalidRpcData));
        }

        soundFontRepository()->addSoundFont(uri);

        return make_response_ret(msg, make_ok());
    });

    onLongRequest(GLOBAL_CTX_ID, MsgCode::AddSoundFontData, [this](const Msg& msg) {
        synth::SoundFontUri uri;
        ByteArray data;
        IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, uri, data)) {
            return make_response_ret(msg, make_ret(Err::InvalidRpcData));
        }

        soundFontRepository()->addSoundFontData(uri, data);

        return make_response_ret(msg, make_ok());
    });

    // Engine Conf
    onLongRequest(GLOBAL_CTX_ID, MsgCode::EngineConfigChanged, [this](const Msg& msg) {
        AudioEngineConfig conf;
        IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, conf)) {
            return make_response_ret(msg, make_ret(Err::InvalidRpcData));
        }

        configuration()->setConfig(conf);

        return make_response_ret(msg, make_ok());
    });

    // AudioContext

    // Init / Deinit
    onLongRequest(GLOBAL_CTX_ID, MsgCode::ContextInit, [this](const Msg& msg) {
        ONLY_AUDIO_RPC_THREAD;

        rpc::CtxId ctxId = 0;
        IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, ctxId)) {
            return make_response_ret(msg, make_ret(Err::InvalidRpcData));
        }

        RetVal<std::shared_ptr<IAudioContext> > ret = audioEngine()->addAudioContext(ctxId);
        if (!ret.ret || !ret.val) {
            return make_response_ret(msg, ret.ret);
        }
        auto acontext = ret.val;

        // Requests to audio context

        // Resources
        onQuickRequest(ctxId, MsgCode::GetAvailableInputResources, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                AudioResourceMetaList list = actx->availableInputResources();
                return make_response_ret(msg, RetVal<AudioResourceMetaList>::make_ok(list));
            } else {
                return make_response_ret(msg, RetVal<AudioResourceMetaList>::make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::GetAvailableSoundPresets, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            AudioResourceMeta meta;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, meta)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }
            if (auto actx = audioContext(msg.ctxId)) {
                SoundPresetList list = actx->availableSoundPresets(meta);
                return make_response_ret(msg, RetVal<SoundPresetList>::make_ok(list));
            } else {
                return make_response_ret(msg, RetVal<SoundPresetList>::make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::GetAvailableOutputResources, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                AudioResourceMetaList list = actx->availableOutputResources();
                return make_response_ret(msg, RetVal<AudioResourceMetaList>::make_ok(list));
            } else {
                return make_response_ret(msg, RetVal<AudioResourceMetaList>::make_ret(Err::InvalidContext));
            }
        });

        // Tracks
        onLongRequest(ctxId, MsgCode::AddTrackWithPlaybackData, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;

            using RetType = RetVal2<TrackId, TrackParams>;

            TrackName trackName;
            mpe::PlaybackData playbackData;
            TrackParams params;
            rpc::StreamId mainStreamId = 0;
            rpc::StreamId offStreamId = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackName, playbackData, params, mainStreamId, offStreamId)) {
                return make_response(msg, RpcPacker::pack(RetType::make_ret(Err::InvalidRpcData)));
            }

            RpcStreamExec mainExec = [this](const std::function<void()>& func) {
                IF_ASSERT_FAILED(m_execOperation) {
                    func();
                    return;
                }
                m_execOperation->execOperation(OperationType::QuickOperation, func);
            };

            RpcStreamExec offExec = [this](const std::function<void()>& func) {
                IF_ASSERT_FAILED(m_execOperation) {
                    func();
                    return;
                }
                m_execOperation->execOperation(OperationType::QuickOperation, func);
            };

            channel()->addReceiveStream(StreamName::PlaybackDataMainStream, mainStreamId, playbackData.mainStream,
                                        mainExec);
            channel()->addReceiveStream(StreamName::PlaybackDataOffStream, offStreamId, playbackData.offStream,
                                        offExec);

            auto addTrackAndSendResponse = [this](const Msg& msg, const TrackName& trackName,
                                                  const mpe::PlaybackData& playbackData, const TrackParams& params) {
                if (auto actx = audioContext(msg.ctxId)) {
                    RetType ret = actx->addTrack(trackName, playbackData, params);
                    channel()->send(make_response(msg, RpcPacker::pack(ret)));
                } else {
                    channel()->send(make_response(msg, RpcPacker::pack(RetType::make_ret(Err::InvalidContext))));
                }
            };

            AudioResourceType resourceType = params.source.resourceMeta.type;

            // Not Fluid
            if (resourceType != AudioResourceType::FluidSoundfont) {
                addTrackAndSendResponse(msg, trackName, playbackData, params);
                return make_response_delayed(msg);
            }

            // Fluid
            std::string sfname = params.source.resourceMeta.attributeVal(synth::SOUNDFONT_NAME_ATTRIBUTE).toStdString();
            if (sfname.empty()) {
                sfname = params.source.resourceMeta.id;
            }

            if (soundFontRepository()->isSoundFontLoaded(sfname)) {
                addTrackAndSendResponse(msg, trackName, playbackData, params);
                return make_response_delayed(msg);
            }
            // Waiting for SF to load
            else if (soundFontRepository()->isLoadingSoundFonts()) {
                LOGI() << "Waiting for SF to load, trackName: " << trackName << ", SF name: " << sfname;
                m_pendingTracks[sfname].emplace_back(PendingTrack { msg, trackName, playbackData, params });

                //! NOTE We subscribe for the first track for which a soundfont is not found.
                //! When the notification is triggered, processing will be called for all tracks.
                if (!m_soundFontsChangedSubscribed) {
                    m_soundFontsChangedSubscribed = true;
                    soundFontRepository()->soundFontsChanged().onNotify(this, [this, addTrackAndSendResponse]() {
                        std::vector<std::string> toRemove;
                        for (auto& p : m_pendingTracks) {
                            const std::string& sfname = p.first;
                            if (soundFontRepository()->isSoundFontLoaded(sfname)) {
                                for (const PendingTrack& t : p.second) {
                                    addTrackAndSendResponse(t.msg, t.trackName, t.playbackData, t.params);
                                }
                                toRemove.push_back(sfname);
                            }
                        }

                        for (const std::string& sf : toRemove) {
                            m_pendingTracks.erase(sf);
                        }

                        if (m_pendingTracks.empty()) {
                            soundFontRepository()->soundFontsChanged().disconnect(this);
                            m_soundFontsChangedSubscribed = false;
                        }
                    });
                }
            } else { // Attempt to add it anyway (most likely fallback will be used)
                addTrackAndSendResponse(msg, trackName, playbackData, params);
            }

            return make_response_delayed(msg);
        });

        onLongRequest(ctxId, MsgCode::AddTrackWithIODevice, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;

            using RetType = RetVal2<TrackId, TrackParams>;

            TrackName trackName;
            uint64_t devicePtr = 0;
            TrackParams params;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackName, devicePtr, params)) {
                return make_response(msg, RpcPacker::pack(RetType::make_ret(Err::InvalidRpcData)));
            }
            io::IODevice* device = reinterpret_cast<io::IODevice*>(devicePtr);

            if (auto actx = audioContext(msg.ctxId)) {
                RetType ret = actx->addTrack(trackName, device, params);
                return make_response(msg, RpcPacker::pack(ret));
            } else {
                return make_response(msg, RpcPacker::pack(RetType::make_ret(Err::InvalidContext)));
            }
        });

        onLongRequest(ctxId, MsgCode::AddAuxTrack, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;

            using RetType = RetVal2<TrackId, TrackParams>;

            TrackName trackName;
            TrackParams params;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackName, params)) {
                return make_response(msg, RpcPacker::pack(RetType::make_ret(Err::InvalidRpcData)));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                RetType ret = actx->addAuxTrack(trackName, params);
                return make_response(msg, RpcPacker::pack(ret));
            } else {
                return make_response(msg, RpcPacker::pack(RetType::make_ret(Err::InvalidContext)));
            }
        });

        onLongRequest(ctxId, MsgCode::RemoveTrack, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            TrackId trackId = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackId)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                actx->removeTrack(trackId);
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onLongRequest(ctxId, MsgCode::RemoveAllTracks, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                actx->removeAllTracks();
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        acontext->trackAdded().onReceive(this, [this, ctxId](TrackId trackId) {
            channel()->send(rpc::make_notification(ctxId, MsgCode::TrackAdded, RpcPacker::pack(trackId)));
        });

        acontext->trackRemoved().onReceive(this, [this, ctxId](TrackId trackId) {
            channel()->send(rpc::make_notification(ctxId, MsgCode::TrackRemoved, RpcPacker::pack(trackId)));
        });

        onQuickRequest(ctxId, MsgCode::GetTrackIdList, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                RetVal<TrackIdList> ret = actx->trackIdList();
                return make_response_ret(msg, ret);
            } else {
                return make_response_ret(msg, RetVal<TrackIdList>::make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::GetTrackName, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;

            TrackId trackId = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackId)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                RetVal<TrackName> ret = actx->trackName(trackId);
                return make_response_ret(msg, ret);
            } else {
                return make_response_ret(msg, RetVal<TrackName>::make_ret(Err::InvalidContext));
            }
        });

        // Params

        onQuickRequest(ctxId, MsgCode::GetTrackParams, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;

            using RetType = RetVal<TrackParams>;

            TrackId trackId = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackId)) {
                return make_response_ret(msg, RetType::make_ret(Err::InvalidRpcData));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                RetType ret = actx->params(trackId);
                return make_response_ret(msg, ret);
            } else {
                return make_response_ret(msg, RetType::make_ret(Err::InvalidContext));
            }
        });

        onLongRequest(ctxId, MsgCode::SetSourceParams, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            TrackId trackId = 0;
            AudioSourceParams params;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackId, params)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                actx->setSourceParams(trackId, params);
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onLongRequest(ctxId, MsgCode::SetControlParams, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            TrackId trackId = 0;
            ControlParams params;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackId, params)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                actx->setControlParams(trackId, params);
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onLongRequest(ctxId, MsgCode::SetFxChainParams, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            TrackId trackId = 0;
            AudioFxChain params;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackId, params)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                actx->setFxChainParams(trackId, params);
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onLongRequest(ctxId, MsgCode::SetAuxSendsParams, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            TrackId trackId = 0;
            AuxSendsParams params;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackId, params)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                actx->setAuxSendsParams(trackId, params);
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        acontext->fxChainParamsChanged().onReceive(this, [this, ctxId](TrackId trackId, const AudioFxChain& params) {
            channel()->send(rpc::make_notification(ctxId, MsgCode::FxChainParamsChanged,
                                                   RpcPacker::pack(trackId, params)));
        });

        acontext->sourceParamsChanged().onReceive(this,
                                                  [this, ctxId](TrackId trackId, const AudioSourceParams& params) {
            channel()->send(rpc::make_notification(ctxId, MsgCode::SourceParamsChanged,
                                                   RpcPacker::pack(trackId, params)));
        });

        // Input processing
        onLongRequest(ctxId, MsgCode::ProcessInput, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            TrackId trackId = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackId)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                actx->processInput(trackId);
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::GetInputProcessingProgress, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            TrackId trackId = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackId)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                RetVal<InputProcessingProgress> ret = actx->inputProcessingProgress(trackId);
                StreamId streamId = 0;
                if (ret.ret) {
                    streamId
                        = channel()->addSendStream(StreamName::InputProcessingProgressStream, ret.val.processedChannel);
                }
                return make_response(msg, RpcPacker::pack(ret.ret, ret.val.isStarted, streamId));
            } else {
                return make_response(msg, RpcPacker::pack(make_ret(Err::InvalidContext), false, 0));
            }
        });

        onLongRequest(ctxId, MsgCode::ClearCache, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            TrackId trackId = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackId)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                actx->clearCache(trackId);
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onLongRequest(ctxId, MsgCode::ClearSources, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                actx->clearSources();
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::ClearMasterOutputParams, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                actx->clearMasterOutputParams();
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onLongRequest(ctxId, MsgCode::ClearAllFx, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                actx->clearAllFx();
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        // Signals
        onQuickRequest(ctxId, MsgCode::GetSignalChanges, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            TrackId trackId = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, trackId)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                RetVal<AudioSignalChanges> ret = audioContext(msg.ctxId)->signalChanges(trackId);
                StreamId streamId = 0;
                if (ret.ret) {
                    streamId = channel()->addSendStream(StreamName::AudioSignalStream, ret.val);
                }

                RetVal<StreamId> res;
                res.ret = ret.ret;
                res.val = streamId;
                return make_response_ret(msg, res);
            } else {
                return make_response_ret(msg, RetVal<StreamId>::make_ret(Err::InvalidContext));
            }
        });

        // Play
        onQuickRequest(ctxId, MsgCode::PrepareToPlay, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                actx->prepareToPlay().onResolve(this, [this, msg](const Ret& ret) {
                    channel()->send(make_response_ret(msg, ret));
                });
                return make_response_delayed(msg);
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::Play, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            secs_t delay = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, delay)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }

            if (auto actx = audioContext(msg.ctxId)) {
                actx->play(delay);
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::Seek, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            secs_t newPosition = 0;
            bool flushSound = false;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, newPosition, flushSound)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }
            if (auto actx = audioContext(msg.ctxId)) {
                actx->seek(newPosition, flushSound);
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::Stop, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                actx->stop();
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::Pause, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                actx->pause();
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::Resume, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            secs_t delay = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, delay)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }
            if (auto actx = audioContext(msg.ctxId)) {
                actx->resume(delay);
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::SetDuration, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            secs_t duration = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, duration)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }
            if (auto actx = audioContext(msg.ctxId)) {
                actx->setDuration(duration);
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::SetLoop, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            secs_t from = 0;
            secs_t to = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, from, to)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }
            if (auto actx = audioContext(msg.ctxId)) {
                Ret ret = actx->setLoop(from, to);
                return make_response_ret(msg, ret);
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::ResetLoop, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                actx->resetLoop();
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::GetPlaybackStatus, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;

            if (auto actx = audioContext(msg.ctxId)) {
                PlaybackStatus status = actx->playbackStatus();
                async::Channel<PlaybackStatus> ch = actx->playbackStatusChanged();
                StreamId streamId = channel()->addSendStream(StreamName::PlaybackStatusStream, ch);
                return make_response(msg, RpcPacker::pack(make_ok(), status, streamId));
            } else {
                return make_response(msg, RpcPacker::pack(make_ret(Err::InvalidContext), PlaybackStatus::Stopped, 0));
            }
        });

        onQuickRequest(ctxId, MsgCode::GetPlaybackPosition, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;

            if (auto actx = audioContext(msg.ctxId)) {
                secs_t pos = actx->playbackPosition();
                async::Channel<secs_t> ch = actx->playbackPositionChanged();
                StreamId streamId = channel()->addSendStream(StreamName::PlaybackPositionStream, ch);
                return make_response(msg, RpcPacker::pack(make_ok(), pos, streamId));
            } else {
                return make_response(msg, RpcPacker::pack(make_ret(Err::InvalidContext), 0, 0));
            }
        });

        // Export

        onLongRequest(ctxId, MsgCode::SaveSoundTrack, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            SoundTrackFormat format;
            uintptr_t dstDevicePtr = 0;
            IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, format, dstDevicePtr)) {
                return make_response_ret(msg, make_ret(Err::InvalidRpcData));
            }
            io::IODevice& dstDevice = *reinterpret_cast<io::IODevice*>(dstDevicePtr);
            if (auto actx = audioContext(msg.ctxId)) {
                actx->saveSoundTrack(dstDevice, format).onResolve(this, [this, msg](const Ret& ret) {
                    channel()->send(make_response_ret(msg, ret));
                });
                return make_response_delayed(msg);
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onLongRequest(ctxId, MsgCode::AbortSavingAllSoundTracks, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                actx->abortSavingAllSoundTracks();
                return make_response_ret(msg, make_ok());
            } else {
                return make_response_ret(msg, make_ret(Err::InvalidContext));
            }
        });

        onQuickRequest(ctxId, MsgCode::GetSaveSoundTrackProgress, [this](const Msg& msg) {
            ONLY_AUDIO_RPC_THREAD;
            if (auto actx = audioContext(msg.ctxId)) {
                SaveSoundTrackProgress ch = actx->saveSoundTrackProgressChanged();
                ch.onReceive(this, [this](int64_t current, int64_t total, SaveSoundTrackStage stage) {
                    ONLY_AUDIO_RPC_THREAD;
                    m_saveSoundTrackProgressStream.send(current, total, stage);
                });

                if (m_saveSoundTrackProgressStreamId == 0) {
                    m_saveSoundTrackProgressStreamId
                        = channel()->addSendStream(StreamName::SaveSoundTrackProgressStream,
                                                   m_saveSoundTrackProgressStream);
                }

                return make_response_ret(msg, RetVal<StreamId>::make_ok(m_saveSoundTrackProgressStreamId));
            } else {
                return make_response_ret(msg, RetVal<StreamId>::make_ret(Err::InvalidContext));
            }
        });

        // response on ContextInit request
        return make_response_ret(msg, ret.ret);
    });

    onQuickRequest(GLOBAL_CTX_ID, MsgCode::ContextDeinit, [this](const Msg& msg) {
        ONLY_AUDIO_RPC_THREAD;

        rpc::CtxId ctxId = 0;
        IF_ASSERT_FAILED(RpcPacker::unpack(msg.data, ctxId)) {
            return make_response_ret(msg, make_ret(Err::InvalidRpcData));
        }

        auto audioContext = this->audioContext(ctxId);
        IF_ASSERT_FAILED(audioContext) {
            return make_response_ret(msg, make_ret(Err::InvalidContext));
        }

        audioContext->trackAdded().disconnect(this);
        audioContext->trackRemoved().disconnect(this);
        audioContext->sourceParamsChanged().disconnect(this);
        audioContext->fxChainParamsChanged().disconnect(this);

        audioEngine()->destroyContext(ctxId);

        return make_response_ret(msg, make_ok());
    });
}

void EngineRpcController::onLongRequest(rpc::CtxId ctxId, rpc::MsgCode code, const RequestHandler& h)
{
    onRequest(OperationType::LongOperation, ctxId, code, h);
}

void EngineRpcController::onQuickRequest(rpc::CtxId ctxId, rpc::MsgCode code, const RequestHandler& h)
{
    onRequest(OperationType::QuickOperation, ctxId, code, h);
}

void EngineRpcController::onRequest(OperationType type, rpc::CtxId ctxId, rpc::MsgCode code,
                                    const RequestHandler& handler)
{
    m_usedRequests.push_back({ ctxId, code });

    channel()->onRequest(ctxId, code, [this, type, code, handler](const Msg& msg) -> Msg {
        Msg resp;
        Operation func = [this, code, handler, msg, &resp]() {
            if (m_terminated) {
                return;
            }

            UNUSED(code);
            BEGIN_METHOD_DURATION;
            resp = handler(msg);
            END_METHOD_DURATION(code);
        };

        IF_ASSERT_FAILED(m_execOperation) {
            func();
        } else {
            m_execOperation->execOperation(type, func);
        }
        return resp;
    });
}

void EngineRpcController::deinit()
{
    ONLY_AUDIO_RPC_THREAD;

    m_terminated = true;

    for (const rpc::MsgKey& key : m_usedRequests) {
        channel()->onRequest(key.ctxId, key.code, nullptr);
    }
    m_usedRequests.clear();
}
