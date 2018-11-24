//
// Copyright (c) 2018 Damian Jarek (damian dot jarek93 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/djarek/ufiber
//

#ifndef UFIBER_UFIBER_HPP
#define UFIBER_UFIBER_HPP

#include <boost/context/fiber.hpp>
#include <ufiber/detail/ufiber.hpp>

#include <atomic>

namespace ufiber
{

struct broken_promise final : std::exception
{
    UFIBER_INLINE_DECL char const* what() const noexcept final;
};

template<class Executor, std::size_t slab_size>
class yield_token
{
public:
    template<class E, std::size_t N>
    yield_token(yield_token<E, N> const& other)
      : ctx_{other.ctx_}
      , executor_{other.executor_}
    {
    }

    using executor_type = Executor;

    yield_token(Executor const& ex, detail::fiber_context&);

    executor_type get_executor() noexcept;

    template<class E, std::size_t N>
    friend detail::fiber_context& detail::get_fiber(yield_token<E, N>& yt);

    template<class E, std::size_t N>
    friend class yield_token;

private:
    detail::fiber_context& ctx_;
    Executor executor_;
};

template<class Executor, class F>
auto
spawn(Executor const& ex, F&& f) ->
  typename std::enable_if<boost::asio::is_executor<Executor>::value>::type;

template<class ExecutionContext, class F>
auto
spawn(ExecutionContext& e, F&& f) -> typename std::enable_if<
  std::is_convertible<ExecutionContext&,
                      boost::asio::execution_context&>::value>::type;

template<class StackAllocator, class Executor, class F>
void
spawn(std::allocator_arg_t arg, StackAllocator&& sa, Executor const& ex, F&& f);

} // namespace ufiber

#include <ufiber/impl/ufiber.hpp>

#ifndef UFIBER_SEPARATE_COMPILATION
#include <ufiber/impl/ufiber.ipp>
#endif // UFIBER_SEPARATE_COMPILATION

#endif // UFIBER_UFIBER_HPP
