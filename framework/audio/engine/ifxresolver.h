/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

#include <memory>

#include "modularity/imoduleinterface.h"

#include "audio/common/audiotypes.h"
#include "ifxprocessor.h"

namespace muse::audio::fx {
class IFxResolver : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(IFxResolver)

public:
    virtual ~IFxResolver() = default;

    class IResolver
    {
    public:
        virtual ~IResolver() = default;

        virtual std::vector<IFxProcessorPtr> resolveFxList(const audio::TrackId trackId, const AudioFxChain& fxChain,
                                                           const OutputSpec& outputSpec) = 0;
        virtual std::vector<IFxProcessorPtr> resolveMasterFxList(const AudioFxChain& fxChain, const OutputSpec& outputSpec) = 0;
        virtual AudioResourceMetaList resolveResources() const = 0;

        //! NOTE Fresh, uncached instances (see IFxResolver::createFxListCopy)
        virtual std::vector<IFxProcessorPtr> createFxListCopy(const audio::TrackId copyId, const AudioFxChain& fxChain,
                                                              const OutputSpec& outputSpec) = 0;
        virtual void releaseFxListCopy(const audio::TrackId copyId, const AudioFxChain& fxChain) = 0;

        virtual void refresh() = 0;
        virtual void clearAllFx() = 0;
    };
    using IResolverPtr = std::shared_ptr<IResolver>;

    virtual std::vector<IFxProcessorPtr> resolveMasterFxList(const AudioFxChain& fxChain, const OutputSpec& outputSpec) = 0;
    virtual std::vector<IFxProcessorPtr> resolveFxList(const TrackId trackId, const AudioFxChain& fxChain,
                                                       const OutputSpec& outputSpec) = 0;
    virtual AudioResourceMetaList resolveAvailableResources() const = 0;

    //! NOTE Unlike resolveFxList(), which returns the instances cached for a track (so asking twice
    //! returns the same processors), this always creates new instances with the same settings.
    //! Used to give each worker of a parallel export its own copy of a shared bus (aux/master).
    //! copyId must be a unique id that is not used by any real track; release the copies with
    //! releaseFxListCopy() using the same copyId and chain.
    virtual std::vector<IFxProcessorPtr> createFxListCopy(const TrackId copyId, const AudioFxChain& fxChain,
                                                          const OutputSpec& outputSpec) = 0;
    virtual void releaseFxListCopy(const TrackId copyId, const AudioFxChain& fxChain) = 0;

    virtual void registerResolver(const AudioFxType type, IResolverPtr resolver) = 0;
    virtual void clearAllFx() = 0;
};

using IFxResolverPtr = std::shared_ptr<IFxResolver>;
}
