/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
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
#include <gtest/gtest.h>

#include <memory>

#ifndef MODULENAME
#define MODULENAME "test"
#endif

#include "thirdparty/kors_modularity/modularity/modulesioc.h"

using namespace kors::modularity;

namespace {
class IServiceA : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(test::IServiceA)
};

class IServiceB : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(test::IServiceB)
};

class ServiceA : public IServiceA
{
};
class ServiceB : public IServiceB
{
};
}

TEST(Global_ModulesIoCTests, Reset_NotifiesSubscribersWithNull)
{
    ModulesGlobalIoC ioc;
    ioc.registerExport<IServiceA>("test", std::make_shared<ServiceA>());

    Subscriber<IServiceA> subscriber;
    int calls = 0;
    std::shared_ptr<IServiceA> last = std::make_shared<ServiceA>();
    subscriber.onChanged = [&](const std::shared_ptr<IServiceA>& p) {
        ++calls;
        last = p;
    };
    ioc.subscribe<IServiceA>(&subscriber);

    ioc.reset();

    EXPECT_EQ(calls, 1);
    EXPECT_EQ(last, nullptr);
    EXPECT_EQ(ioc.resolve<IServiceA>(""), nullptr);
}

//! NOTE While the subscribers are notified, a callback can destroy an object that holds another subscriber.
//! Such a subscriber unsubscribes, and its callback must not be called after that (it points to a destroyed object)
TEST(Global_ModulesIoCTests, Reset_SubscriberUnsubscribedDuringReset_NotNotified)
{
    ModulesGlobalIoC ioc;
    ioc.registerExport<IServiceA>("test", std::make_shared<ServiceA>());
    ioc.registerExport<IServiceB>("test", std::make_shared<ServiceB>());

    Subscriber<IServiceA> subscriberA;
    Subscriber<IServiceB> subscriberB;
    int callsB = 0;

    //! Services are notified in the order of their ids, A is first
    subscriberA.onChanged = [&](const std::shared_ptr<IServiceA>&) {
        subscriberB.unsubscribe();
    };
    subscriberB.onChanged = [&](const std::shared_ptr<IServiceB>&) {
        ++callsB;
    };

    ioc.subscribe<IServiceA>(&subscriberA);
    ioc.subscribe<IServiceB>(&subscriberB);

    ioc.reset();

    EXPECT_EQ(callsB, 0);
}

TEST(Global_ModulesIoCTests, Reset_ServicesReleased)
{
    ModulesGlobalIoC ioc;
    auto service = std::make_shared<ServiceA>();
    std::weak_ptr<ServiceA> weak = service;
    ioc.registerExport<IServiceA>("test", std::move(service));

    ioc.reset();

    EXPECT_TRUE(weak.expired());
}
