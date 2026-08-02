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

#include "audiofactory.h"

#include "audio/common/audioerrors.h"

#include "nodes/eventaudionode.h"

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::engine;

AudioResourceMetaList AudioFactory::availableInputResources() const
{
    return synthResolver()->resolveAvailableResources();
}

SoundPresetList AudioFactory::availableSoundPresets(const AudioResourceMeta& resourceMeta) const
{
    return synthResolver()->resolveAvailableSoundPresets(resourceMeta);
}

AudioResourceMetaList AudioFactory::availableOutputResources() const
{
    return fxResolver()->resolveAvailableResources();
}

RetVal<synth::ISynthesizerPtr> AudioFactory::makeSynth(const TrackId trackId, const AudioInputParams& params,
                                                       const PlaybackSetupData& setupData) const
{
    auto synth = synthResolver()->resolveSynth(trackId, params, audioEngine()->outputSpec(), setupData);
    if (!synth) {
        return RetVal<synth::ISynthesizerPtr>::make_ret(Err::InvalidSynth);
    }
    return RetVal<synth::ISynthesizerPtr>::make_ok(synth);
}

RetVal<synth::ISynthesizerPtr> AudioFactory::makeDefaultSynth(const TrackId trackId) const
{
    auto synth = synthResolver()->resolveDefaultSynth(trackId);
    if (!synth) {
        return RetVal<synth::ISynthesizerPtr>::make_ret(Err::InvalidSynth);
    }
    return RetVal<synth::ISynthesizerPtr>::make_ok(synth);
}

void AudioFactory::clearSynthSources()
{
    synthResolver()->clearSources();
}

RetVal<AudioSourceNodePtr> AudioFactory::makeEventSource(const TrackId trackId, const std::string& trackName,
                                                         const mpe::PlaybackData& playbackData,
                                                         const AudioInputParams& params,
                                                         const std::function<void()> onOffStreamReceived) const
{
    EventAudioNodePtr source = std::make_shared<EventAudioNode>(trackId, trackName, playbackData, onOffStreamReceived);
    source->setOutputSpec(audioEngine()->outputSpec());
    source->applyInputParams(params);
    return RetVal<AudioSourceNodePtr>::make_ok(source);
}

FxChainPtr AudioFactory::makeMasterFxChain(const AudioFxChain& fxChain) const
{
    std::vector<IFxProcessorPtr> fxlist = fxResolver()->resolveMasterFxList(fxChain, audioEngine()->outputSpec());

    FxChainPtr chain = std::make_shared<FxChain>();
    chain->setName("FxChain [master]");
    chain->setFxList(fxlist);
    chain->setFxChainSpec(fxChain);

    return chain;
}

FxChainPtr AudioFactory::makeTrackFxChain(const TrackId trackId, const AudioFxChain& fxChain) const
{
    std::vector<IFxProcessorPtr> fxlist = fxResolver()->resolveFxList(trackId, fxChain, audioEngine()->outputSpec());

    FxChainPtr chain = std::make_shared<FxChain>();
    if (trackId == MASTER_TRACK_ID) {
        chain->setName("FxChain [master]");
    } else {
        chain->setName("FxChain [track " + std::to_string(trackId) + "]");
    }
    chain->setFxList(fxlist);
    chain->setFxChainSpec(fxChain);

    return chain;
}

void AudioFactory::clearAllFx()
{
    fxResolver()->clearAllFx();
}
