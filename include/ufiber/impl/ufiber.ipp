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

#include <ufiber/ufiber.hpp>

#include <cassert>

namespace ufiber
{

char const*
broken_promise::what() const noexcept
{
    return "Broken fiber promise";
}

namespace detail
{

#ifndef BOOST_NO_EXCEPTIONS
[[noreturn]] void
throw_broken_promise()
{
    // If you disable exceptions you'll need to provide your own definition of
    // this function, which is called when an asynchronous operation is
    // abandoned (e.g. during a hard shutdown of an `io_context`).
    throw broken_promise{};
}
#endif // BOOST_NO_EXCEPTIONS

fiber_context::fiber_context(boost::context::fiber&& f)
  : fiber_{std::move(f)}
{
}

void
fiber_context::resumer::operator()(void*) noexcept
{
    // Move onto stack, because resume() may invalidate ctx if fiber terminates
    auto fiber = std::move(ctx_.fiber_);
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

void
promise<>::set_result() noexcept
{
    state_ = result_state::value;
}

void
promise<>::get_value()
{
    switch (state_)
    {
        case result_state::value:
            return;
        default:
            throw broken_promise{};
    }
}

} // detail
} // ufiber

#endif // UFIBER_IMPL_UFIBER_IPP
