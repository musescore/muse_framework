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
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QFileOpenEvent>
#include <QObject>
#include <QUrl>

#include "global/internal/applicationeventcontroller.h"

using namespace muse;

class Global_ApplicationEventControllerTests : public ::testing::Test
{
public:

    void SetUp() override
    {
        m_controller = std::make_unique<ApplicationEventController>();
    }

    void TearDown() override
    {
        m_controller.reset();
    }

    void send(QEvent* event)
    {
        QCoreApplication::sendEvent(&m_target, event);
    }

    static QUrl urlOf(const std::unique_ptr<QEvent>& event)
    {
        return static_cast<QFileOpenEvent*>(event.get())->url();
    }

    std::unique_ptr<ApplicationEventController> m_controller;
    QObject m_target;
};

TEST_F(Global_ApplicationEventControllerTests, ConfiguredTypeIsBuffered)
{
    m_controller->setPendingEventTypes({ QEvent::FileOpen });

    const QUrl url("file:///tmp/project.aup4");
    QFileOpenEvent event(url);
    send(&event);

    const std::vector<std::unique_ptr<QEvent> > pending = m_controller->takePendingEvents();

    ASSERT_EQ(pending.size(), 1u);
    EXPECT_EQ(pending.at(0)->type(), QEvent::FileOpen);
    EXPECT_EQ(urlOf(pending.at(0)), url);
    EXPECT_NE(pending.at(0).get(), &event);
}

TEST_F(Global_ApplicationEventControllerTests, UnconfiguredTypeIsNotBuffered)
{
    m_controller->setPendingEventTypes({ QEvent::FileOpen });

    QEvent event(QEvent::User);
    send(&event);

    EXPECT_TRUE(m_controller->takePendingEvents().empty());
}

TEST_F(Global_ApplicationEventControllerTests, NothingIsBufferedWithoutConfiguredTypes)
{
    QFileOpenEvent event(QUrl("file:///tmp/project.aup4"));
    send(&event);

    EXPECT_TRUE(m_controller->takePendingEvents().empty());
}

TEST_F(Global_ApplicationEventControllerTests, BufferedEventOutlivesTheOriginal)
{
    m_controller->setPendingEventTypes({ QEvent::FileOpen });

    const QUrl url("file:///tmp/project.aup4");
    {
        QFileOpenEvent event(url);
        send(&event);
    }

    const std::vector<std::unique_ptr<QEvent> > pending = m_controller->takePendingEvents();

    ASSERT_EQ(pending.size(), 1u);
    EXPECT_EQ(pending.at(0)->type(), QEvent::FileOpen);
    EXPECT_EQ(urlOf(pending.at(0)), url);
}

TEST_F(Global_ApplicationEventControllerTests, PendingEventsAreTakenOnlyOnce)
{
    m_controller->setPendingEventTypes({ QEvent::FileOpen });

    QFileOpenEvent event(QUrl("file:///tmp/project.aup4"));
    send(&event);

    EXPECT_EQ(m_controller->takePendingEvents().size(), 1u);
    EXPECT_TRUE(m_controller->takePendingEvents().empty());
}

TEST_F(Global_ApplicationEventControllerTests, BufferingStopsAfterTheFirstTake)
{
    m_controller->setPendingEventTypes({ QEvent::FileOpen });

    QFileOpenEvent beforeTake(QUrl("file:///tmp/before.aup4"));
    send(&beforeTake);

    ASSERT_EQ(m_controller->takePendingEvents().size(), 1u);

    QFileOpenEvent afterTake(QUrl("file:///tmp/after.aup4"));
    send(&afterTake);

    EXPECT_TRUE(m_controller->takePendingEvents().empty());
}
