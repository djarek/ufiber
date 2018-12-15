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

#include <ufiber/detail/ufiber.hpp>

/**
 * @file
 * Main API header of the library.
 */

namespace ufiber
{

/**
 * @mainpage μfiber library
 *
 * @ref ufiber
 */

/**
 * Exception thrown if an asynchronous operation is abandoned. If this exception
 * escapes the fiber's main function, the fiber will complete normally. This
 * behavior is required so that an `io_context` abandoning work does not result
 * in a call to std::terminate.
 *
 * @remark If this exception is thrown, the fiber may have been resumed outside
 * its associated executor's context.
 */
struct broken_promise final : std::exception
{
    UFIBER_INLINE_DECL char const* what() const noexcept final;
};

/**
 * A lightweight handle to the currently running fiber.
 *
 * @remark Use of a token outside the fiber it was created on results in
 * undefined behavior, but it may safely by passed to functions that run on the
 * current fiber's stack.
 *
 * @tparam Executor executor to be used to execute the asynchronous operations.
 */
template<class Executor>
class yield_token
{
public:
    /**
     * Executor type associated with this yield_token object.
     */
    using executor_type = Executor;

    /**
     * Converting constructor.
     * This function participates in overload resolution only if Executor is
     * Constructible from E const&.
     */
    template<class E,
             class = typename std::enable_if<
               std::is_convertible<E&, Executor>::value>::type>
    yield_token(yield_token<E> const& other)
      : ctx_{other.ctx_}
      , executor_{other.executor_}
    {
    }

    /**
     * Returns the executor object associated with this yield_token object.
     */
    executor_type get_executor() noexcept;

private:
    yield_token(Executor const& ex, detail::fiber_context&);

    template<class E>
    friend class yield_token;

    template<class F, class E, class Ex>
    friend struct detail::fiber_main;

    template<class E>
    friend detail::fiber_context& detail::get_fiber(yield_token<E>& yt);

    detail::fiber_context& ctx_;
    Executor executor_;
};

/**
 * Spawns a new fiber on the provided executor. The fiber will invoke a
 * `DECAY_COPY` of F. This function participates in overload resolution if and
 * only if E is an Executor.
 *
 * @param ex the executor that will be associated with the fiber.
 * @param f the function object that will be invoked as the fiber's main
 * function.
 */
template<class E, class F>
auto
spawn(E const& ex, F&& f) ->
  typename std::enable_if<boost::asio::is_executor<E>::value>::type;

/**
 * Spawns a new fiber on the provided ExecutionContext. The fiber will invoke a
 * `DECAY_COPY` of F. This function participates in overload resolution if and
 * only if ExecutionContext is publicly derived from
 * `boost::asio::execution_context`.
 *
 * @param e the ExecutionContext that will be associated with the fiber.
 * @param f the function object that will be invoked as the fiber's main
 * function.
 */
template<class E, class F>
auto
spawn(E& e, F&& f) -> typename std::enable_if<
  std::is_convertible<E&, boost::asio::execution_context&>::value>::type;

/**
 * Spawns a new fiber on the provided executor. The fiber will invoke a
 * `DECAY_COPY` of F. The provided StackAllocator will be used to allocate the
 * fiber's stack. Refer to the StackAllocator concept in boost::context for more
 * information.
 *
 * @param arg std::allocator_arg tag to disambiguate overloads.
 * @param sa an object that satisfies the requirements of the StackAllocator
 * concept.
 * @param ex the executor that will be associated with the fiber.
 * @param f the function object that will be invoked as the fiber's main
 * function.
 */
template<class Alloc, class E, class F>
void
spawn(std::allocator_arg_t arg, Alloc&& sa, E const& ex, F&& f);

} // namespace ufiber

#include <ufiber/impl/ufiber.hpp>

#ifndef UFIBER_SEPARATE_COMPILATION
#include <ufiber/impl/ufiber.ipp>
#endif // UFIBER_SEPARATE_COMPILATION

#endif // UFIBER_UFIBER_HPP
