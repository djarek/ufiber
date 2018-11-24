//
// Copyright (c) 2018 Damian Jarek (damian dot jarek93 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/djarek/ufiber
//

#ifndef UFIBER_IMPL_UFIBER_HPP
#define UFIBER_IMPL_UFIBER_HPP

#include <ufiber/ufiber.hpp>

namespace ufiber
{

template<class Executor, std::size_t slab_size>
yield_token<Executor, slab_size>::yield_token(Executor const& ex,
                                              detail::fiber_context& ctx)
  : ctx_{ctx}
  , executor_{ex}
{
}
template<class Executor, std::size_t slab_size>
typename yield_token<Executor, slab_size>::executor_type
yield_token<Executor, slab_size>::get_executor() noexcept
{
    return executor_;
}

template<class Executor, class F>
auto
spawn(Executor const& ex, F&& f) ->
  typename std::enable_if<boost::asio::is_executor<Executor>::value>::type
{
    detail::initial_resume(boost::context::fiber{
      detail::fiber_main<typename std::decay<F>::type, Executor>{
        std::forward<F>(f), ex}});
}

template<class ExecutionContext, class F>
auto
spawn(ExecutionContext& e, F&& f) -> typename std::enable_if<
  std::is_convertible<ExecutionContext&,
                      boost::asio::execution_context&>::value>::type
{
    detail::initial_resume(boost::context::fiber{
      detail::fiber_main<typename std::decay<F>::type,
                         typename ExecutionContext::executor_type>{
        std::forward<F>(f), e.get_executor()}});
}

template<class StackAllocator, class Executor, class F>
void
spawn(std::allocator_arg_t arg, StackAllocator&& sa, Executor const& ex, F&& f)
{
    detail::initial_resume(boost::context::fiber{
      arg,
      std::forward<StackAllocator>(sa),
      detail::fiber_main<typename std::decay<F>::type, Executor>{
        std::forward<F>(f), ex}});
}
} // namespace ufiber

#endif // UFIBER_IMPL_UFIBER_HPP
