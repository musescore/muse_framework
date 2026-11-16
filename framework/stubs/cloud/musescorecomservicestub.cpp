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
#include "musescorecomservicestub.h"

#include "async/async.h"

#include "authorizationservicestub.h"

using namespace muse;
using namespace muse::cloud;

template<typename T>
static async::Promise<RetVal<T> > notSupportedRetVal()
{
    return async::make_promise<RetVal<T> >([](auto resolve) {
        return resolve(RetVal<T>::make_ret(Ret::Code::NotSupported));
    });
}

static async::Promise<Ret> notSupportedRet()
{
    return async::make_promise<Ret>([](auto resolve) {
        return resolve(make_ret(Ret::Code::NotSupported));
    });
}

//! NOTE The progress is finished asynchronously, so that the caller has time to subscribe to it
static ProgressPtr notSupportedProgress(const async::Asyncable* caller)
{
    ProgressPtr progress = std::make_shared<Progress>();
    async::Async::call(caller, [progress]() {
        //! NOTE A canceled progress is already finished
        if (progress->isCanceled()) {
            return;
        }
        progress->finish(make_ret(Ret::Code::NotSupported));
    });

    return progress;
}

// MuseScoreComConvertServiceStub

async::Promise<RetVal<ConvertConfig> > MuseScoreComConvertServiceStub::fetchConfig()
{
    return notSupportedRetVal<ConvertConfig>();
}

ProgressPtr MuseScoreComConvertServiceStub::uploadToConvert(const ConvertUploadDataPtr&)
{
    return notSupportedProgress(this);
}

async::Promise<RetVal<ConvertQueueList> > MuseScoreComConvertServiceStub::fetchQueue()
{
    return notSupportedRetVal<ConvertQueueList>();
}

async::Promise<RetVal<ConvertResult> > MuseScoreComConvertServiceStub::submitReview(ConvertType, int, ReviewRating, const QString&)
{
    return notSupportedRetVal<ConvertResult>();
}

async::Promise<Ret> MuseScoreComConvertServiceStub::submitReviewComment(ConvertType, int, const QString&)
{
    return notSupportedRet();
}

async::Promise<Ret> MuseScoreComConvertServiceStub::deleteConversion(ConvertType, int)
{
    return notSupportedRet();
}

// MuseScoreComServiceStub

MuseScoreComServiceStub::MuseScoreComServiceStub()
    : m_authorization(std::make_shared<AuthorizationServiceStub>()),
    m_convert(std::make_shared<MuseScoreComConvertServiceStub>())
{
}

IAuthorizationServicePtr MuseScoreComServiceStub::authorization()
{
    return m_authorization;
}

IMuseScoreComConvertServicePtr MuseScoreComServiceStub::convert()
{
    return m_convert;
}

QUrl MuseScoreComServiceStub::scoreManagerUrl() const
{
    return QUrl();
}

ProgressPtr MuseScoreComServiceStub::uploadScore(DevicePtr, const QString&, cloud::Visibility, const QUrl&, int)
{
    return notSupportedProgress(this);
}

ProgressPtr MuseScoreComServiceStub::uploadAudio(DevicePtr, const QString&, const QUrl&)
{
    return notSupportedProgress(this);
}

async::Promise<RetVal<ScoreInfo> > MuseScoreComServiceStub::downloadScoreInfo(const QUrl&)
{
    return notSupportedRetVal<ScoreInfo>();
}

async::Promise<RetVal<ScoreInfo> > MuseScoreComServiceStub::downloadScoreInfo(int)
{
    return notSupportedRetVal<ScoreInfo>();
}

async::Promise<ScoresList> MuseScoreComServiceStub::downloadScoresList(int, int)
{
    return async::make_promise<ScoresList>([](auto resolve) {
        return resolve(ScoresList());
    });
}

ProgressPtr MuseScoreComServiceStub::downloadScore(int, DevicePtr, const QString&, const QString&)
{
    return notSupportedProgress(this);
}

async::Promise<Ret> MuseScoreComServiceStub::deleteScore(int)
{
    return notSupportedRet();
}
