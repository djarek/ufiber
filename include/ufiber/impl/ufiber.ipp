//
// Copyright (c) 2018 Damian Jarek (damian dot jarek93 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/djarek/ufiber
//

#ifndef UFIBER_IMPL_UFIBER_IPP
#define UFIBER_IMPL_UFIBER_IPP

#include <ufiber/detail/monotonic_allocator.hpp>
#include <ufiber/ufiber.hpp>

#include <cassert>
#include <thread>

namespace ufiber
{

char const*
broken_promise::what() const noexcept
{
    return "Broken fiber promise";
}

namespace detail
{

fiber_context::fiber_context(boost::context::fiber&& f)
  : fiber_{std::move(f)}
  , running_{true}
{
}

void
fiber_context::suspend() noexcept
{
    fiber_ = std::move(fiber_).resume_with([this](boost::context::fiber&& f) {
        fiber_ = std::move(f);
        running_ = false;
        return boost::context::fiber{};
    });
    running_ = true;
    assert(fiber_ && "Expected caller fiber");
    // fiber_ should contain the main thread's stack at this point
}

void
fiber_context::resumer::operator()(void*) noexcept
{
    while (ctx.running_)
        std::this_thread::yield();
    // Move onto stack, because resume() may invalidate ctx if fiber terminates
    auto fiber = std::move(ctx.fiber_);
    fiber = std::move(fiber).resume();
    // At this point the fiber has either suspended in a different async op
    // or it terminated, so it's impossible for us to get a fiber back here
    assert(!fiber && "Unexpected fiber");
}

boost::context::fiber
fiber_context::final_suspend() noexcept
{
    return std::move(fiber_);
}

void
initial_resume(boost::context::fiber&& f)
{
    f = std::move(f).resume();
    assert(!f && "Unexpected fiber");
}

completion_handler_base::completion_handler_base(
  detail::fiber_context& ctx) noexcept
  : promise_{nullptr, detail::fiber_context::resumer{ctx}}
{
}

completion_handler_base::allocator_type
completion_handler_base::get_allocator() const noexcept
{
    return allocator_;
}

void
completion_handler_base::attach(void* promise,
                                memory_resource_base& mr) noexcept
{
    promise_.reset(promise);
    allocator_ = allocator_type{mr};
}

fiber_context&
completion_handler_base::get_context() const noexcept
{
    return promise_.get_deleter().ctx;
}

monotonic_allocator<void>::monotonic_allocator(
  memory_resource_base& mr) noexcept
  : mr_{&mr}
{
}

void*
monotonic_memory_resource<0>::allocate(std::size_t size, std::size_t)
{
    return ::operator new(size);
}

void
monotonic_memory_resource<0>::deallocate(void* p) noexcept
{
    ::operator delete(p);
}

} // detail
} // ufiber

#endif // UFIBER_IMPL_UFIBER_IPP
