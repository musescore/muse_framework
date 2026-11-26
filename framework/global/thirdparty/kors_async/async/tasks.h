/*
MIT License

Copyright (c) Igor Korsukov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "async.h"
#include "asyncable.h"

namespace kors::async {
template<typename T>
struct Context {
    bool cancel = false;
    size_t current = 0;
    T data;

    Context() = default;
    Context(const T& data)
        : data(data) {}
};

template<typename T>
using Ctx = std::shared_ptr<Context<T> >;

template<typename T>
Ctx<T> make_ctx(const T& data)
{
    return std::make_shared<Context<T> >(data);
}

template<typename T>
class Task
{
public:

    using Call = std::function<void (Ctx<T>)>;
    Task(const Call& func)
        : m_func(func) {}

    void run(Ctx<T> ctx)
    {
        m_func(ctx);
    }

private:
    Call m_func;
};

template<typename T>
using Tasks = std::vector<Task<T> >;

template<typename T>
void processTasks(const Asyncable* caller, const Tasks<T>& tasks, Ctx<T> ctx)
{
    if (ctx->cancel) {
        return;
    }

    if (ctx->current >= tasks.size()) {
        return;
    }

    auto task = tasks.at(ctx->current);

    task.run(ctx);

    if (ctx->cancel) {
        return;
    }

    ctx->current++;
    if (ctx->current >= tasks.size()) {
        return;
    }

    Async::call(caller, [caller, tasks, ctx]() {
        processTasks(caller, tasks, ctx);
    });
}
}
