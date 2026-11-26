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

#include "../thirdparty/kors_async/async/tasks.h"
#include "asyncable.h"

namespace muse::async {
template<typename T>
using Ctx = kors::async::Ctx<T>;

template<typename T>
inline Ctx<T> make_ctx(const T& data)
{
    return kors::async::make_ctx<T>(data);
}

template<typename T>
using Task = kors::async::Task<T>;

template<typename T>
using Tasks = kors::async::Tasks<T>;

template<typename T>
inline void processTasks(const Asyncable* caller, const Tasks<T>& tasks, Ctx<T> ctx)
{
    kors::async::processTasks(caller, tasks, ctx);
}
}
