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

#include "audio/common/audiotypes.h"
#include "audioplugins/audiopluginstypes.h"
#include "musesampler/musesamplertypes.h"
#include "vst/vstpluginattrs.h"

using namespace muse;

static_assert(std::is_same_v<audio::AudioResourceId, std::string>);
static_assert(std::is_same_v<audioplugins::PluginResourceId, std::string>);

static_assert(!std::is_same_v<audio::AudioResourceMeta, audioplugins::PluginMeta>);

TEST(Audio_AudioResourceTypes, WireStringsAreCanonical)
{
    EXPECT_EQ(audio::FLUID_SOUNDFONT_TYPE_NAME,      "FluidSoundfont");
    EXPECT_EQ(audio::NATIVE_EFFECT_TYPE_NAME,        "NativeEffect");
    EXPECT_EQ(vst::AUDIO_RESOURCE_TYPE_NAME,         "VstPlugin");
    EXPECT_EQ(musesampler::AUDIO_RESOURCE_TYPE_NAME, "MuseSamplerSoundPack");
}

TEST(Audio_AudioResourceTypes, ResourceTypeNameMatchesModuleConstants)
{
    EXPECT_EQ(audio::resourceTypeName(audio::AudioResourceType::FluidSoundfont),
              audio::FLUID_SOUNDFONT_TYPE_NAME);
    EXPECT_EQ(audio::resourceTypeName(audio::AudioResourceType::NativeEffect),
              audio::NATIVE_EFFECT_TYPE_NAME);
    EXPECT_EQ(audio::resourceTypeName(audio::AudioResourceType::VstPlugin),
              vst::AUDIO_RESOURCE_TYPE_NAME);
    EXPECT_EQ(audio::resourceTypeName(audio::AudioResourceType::MuseSamplerSoundPack),
              musesampler::AUDIO_RESOURCE_TYPE_NAME);
}

TEST(Audio_AudioResourceTypes, ResourceTypeFromStringRoundTrips)
{
    for (auto type : { audio::AudioResourceType::FluidSoundfont,
                       audio::AudioResourceType::VstPlugin,
                       audio::AudioResourceType::NativeEffect,
                       audio::AudioResourceType::MuseSamplerSoundPack }) {
        EXPECT_EQ(audio::resourceTypeFromString(audio::resourceTypeName(type)), type);
    }
}

TEST(Audio_AudioResourceTypes, ResourceTypeFromStringRejectsUnknown)
{
    EXPECT_EQ(audio::resourceTypeFromString("AudioUnit"),     audio::AudioResourceType::Undefined);
    EXPECT_EQ(audio::resourceTypeFromString("Lv2Plugin"),     audio::AudioResourceType::Undefined);
    EXPECT_EQ(audio::resourceTypeFromString("NyquistPlugin"), audio::AudioResourceType::Undefined);
    EXPECT_EQ(audio::resourceTypeFromString(""),              audio::AudioResourceType::Undefined);
    EXPECT_EQ(audio::resourceTypeFromString("Garbage"),       audio::AudioResourceType::Undefined);
}

TEST(Audio_AudioResourceTypes, IsResourceTypeHelper)
{
    audio::AudioResourceMeta meta;
    meta.type = vst::AUDIO_RESOURCE_TYPE_NAME;
    EXPECT_TRUE(audio::isResourceType(meta, audio::AudioResourceType::VstPlugin));
    EXPECT_FALSE(audio::isResourceType(meta, audio::AudioResourceType::FluidSoundfont));

    meta.type = "AudioUnit";
    EXPECT_FALSE(audio::isResourceType(meta, audio::AudioResourceType::VstPlugin));
    EXPECT_TRUE(audio::isResourceType(meta, audio::AudioResourceType::Undefined));
}

TEST(Audio_AudioResourceTypes, ToAudioResourceIdRoundTrips)
{
    const audioplugins::PluginResourceId plugId = "Muse Reverb";
    const audio::AudioResourceId audioId = audio::toAudioResourceId(plugId);
    EXPECT_EQ(audioId, plugId);
    EXPECT_EQ(audioId, "Muse Reverb");

    const audioplugins::PluginResourceIdList plugIds = { "A", "B", "C" };
    const audio::AudioResourceIdList audioIds = audio::toAudioResourceIdList(plugIds);
    ASSERT_EQ(audioIds.size(), plugIds.size());
    for (size_t i = 0; i < plugIds.size(); ++i) {
        EXPECT_EQ(audioIds[i], plugIds[i]);
    }
}
