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
#include <gtest/gtest.h>

#include "async/processevents.h"
#include "async/tasks.h"

using namespace muse;

class Global_Async_TasksTests : public ::testing::Test
{
public:
};

TEST_F(Global_Async_TasksTests, Task_Run)
{
    //! GIVEN Task with function that returns int
    auto task = async::Task<int>([](async::Ctx<int> ctx) {
        ctx->data = 42;
    });

    //! WHEN Task is run
    async::Ctx<int> ctx = async::make_ctx(0);
    task.run(ctx);

    //! THEN Context data should be 42
    EXPECT_EQ(ctx->data, 42);
}

auto task1 = [](async::Ctx<int> ctx) {
    ctx->data = 1;
};
auto task2 = [](async::Ctx<int> ctx) {
    ctx->data = 2;
};
auto task3 = [](async::Ctx<int> ctx) {
    ctx->data = 3;
};

auto taskCancel = [](async::Ctx<int> ctx) {
    ctx->cancel = true;
};

TEST_F(Global_Async_TasksTests, ProcessTasks_Run)
{
    //! GIVEN Tasks with functions that return int
    async::Tasks<int> tasks =  {
        { task1 },
        { task2 },
        { task3 },
    };

    //! WHEN ProcessTasks is run
    async::Ctx<int> ctx = async::make_ctx(0);
    async::processTasks(nullptr, tasks, ctx);

    //! THEN Context data should be 1
    EXPECT_EQ(ctx->data, 1);

    // next event loop iteration
    async::processMessages();

    //! THEN Context data should be 2
    EXPECT_EQ(ctx->data, 2);

    // next event loop iteration
    async::processMessages();

    //! THEN Context data should be 3
    EXPECT_EQ(ctx->data, 3);
}

TEST_F(Global_Async_TasksTests, ProcessTasks_Cancel)
{
    //! GIVEN Tasks with functions that return int
    async::Tasks<int> tasks =  {
        { task1 },
        { task2 },
        { taskCancel },
        { task3 },
    };

    //! WHEN ProcessTasks is run
    async::Ctx<int> ctx = async::make_ctx(0);
    async::processTasks(nullptr, tasks, ctx);

    //! THEN Context data should be 1
    EXPECT_EQ(ctx->data, 1);

    // next event loop iteration
    async::processMessages();

    //! THEN Context data should be 2
    EXPECT_EQ(ctx->data, 2);

    // next event loop iteration
    async::processMessages();

    //! THEN Context data should be 2 because taskCancel has been called
    EXPECT_EQ(ctx->data, 2);
    EXPECT_TRUE(ctx->cancel);
}

TEST_F(Global_Async_TasksTests, ProcessTasks_DeadCaller)
{
    //! GIVEN Tasks with functions that return int
    async::Tasks<int> tasks =  {
        { task1 },
        { task2 },
        { task3 },
    };

    //! WHEN ProcessTasks is run
    auto caller = new async::Asyncable();
    async::Ctx<int> ctx = async::make_ctx(0);
    async::processTasks(caller, tasks, ctx);

    //! THEN Context data should be 1
    EXPECT_EQ(ctx->data, 1);

    // next event loop iteration
    async::processMessages();

    //! THEN Context data should be 2
    EXPECT_EQ(ctx->data, 2);

    delete caller;

    // next event loop iteration
    async::processMessages();

    //! THEN Context data should be 2 because the caller is dead
    EXPECT_EQ(ctx->data, 2);
}
