//
// Copyright (c) 2018 Damian Jarek (damian dot jarek93 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/djarek/ufiber
//

#ifndef UFIBER_DETAIL_UFIBER_HPP
#define UFIBER_DETAIL_UFIBER_HPP

#include <ufiber/detail/config.hpp>

#include <boost/asio/post.hpp>
#include <boost/context/fiber.hpp>
#include <boost/core/no_exceptions_support.hpp>

#include <atomic>
#include <thread>

namespace ufiber
{

template<class Executor>
class yield_token;

struct broken_promise;

namespace detail
{

class void_fn_ref;

[[noreturn]] UFIBER_INLINE_DECL void
throw_broken_promise();

class fiber_context
{
public:
    struct resumer
    {
        UFIBER_INLINE_DECL void operator()(void* promise) noexcept;
        fiber_context& ctx_;
    };

    UFIBER_INLINE_DECL explicit fiber_context(boost::context::fiber&& f);
    fiber_context(fiber_context&&) = delete;
    fiber_context(fiber_context const&) = delete;

    fiber_context& operator=(fiber_context&&) = delete;
    fiber_context& operator=(fiber_context const&) = delete;

    template<class F>
    void suspend_with(F&& init) noexcept
    {
        fiber_ = std::move(fiber_).resume_with(
          [this, &init](boost::context::fiber&& f) {
              fiber_ = std::move(f);
              init();
              return boost::context::fiber{};
          });
        assert(fiber_ && "Expected caller fiber");
        // fiber_ should contain the main thread's stack at this point
    }

    UFIBER_INLINE_DECL void resume() noexcept;

    UFIBER_INLINE_DECL boost::context::fiber final_suspend() noexcept;

private:
    boost::context::fiber fiber_;
};

template<class Executor>
detail::fiber_context&
get_fiber(yield_token<Executor>& yt)
{
    return yt.ctx_;
}

UFIBER_INLINE_DECL void
initial_resume(boost::context::fiber&& f);

enum class result_state
{
    broken_promise,
    value,
};

template<class... Args>
struct select_result
{
    using type = std::tuple<Args...>;
};

template<class Arg>
struct select_result<Arg>
{
    using type = Arg;
};

template<>
struct select_result<>
{
    using type = void;
};

template<class... Args>
using result_t = typename select_result<Args...>::type;

template<class... Args>
struct promise
{
    using result_type = result_t<Args...>;

    promise()
    {
    }

    promise(promise&&) = delete;
    promise(promise const&) = delete;
    promise& operator=(promise&&) = delete;
    promise& operator=(promise const&) = delete;

    ~promise()
    {
        switch (state_)
        {
            case result_state::broken_promise:
                break;
            case result_state::value:
                result_.~result_type();
                break;
        }
    }

    template<class... Ts>
    void set_result(Ts&&... ts)
    {
        ::new (static_cast<void*>(std::addressof(result_)))
          result_type(std::forward<Ts>(ts)...);
        state_ = result_state::value;
    }

    result_type get_value()
    {
        switch (state_)
        {
            case result_state::value:
                return std::move(result_);
            default:
                detail::throw_broken_promise();
        }
    }

private:
    result_state state_ = result_state::broken_promise;
    union {
        result_type result_;
    };
};

template<>
struct promise<>
{
    promise() = default;
    UFIBER_INLINE_DECL void set_result() noexcept;
    UFIBER_INLINE_DECL void get_value();

private:
    result_state state_ = result_state::broken_promise;
};

template<class Executor, class... Args>
struct completion_handler
{
    explicit completion_handler(void* promise,
                                yield_token<Executor>& yt,
                                fiber_context& ctx)
      : promise_{promise, fiber_context::resumer{ctx}}
      , executor_{yt.get_executor()}
    {
    }

    template<class... Ts>
    void operator()(Ts&&... ts)
    {
        static_cast<promise<Args...>*>(this->promise_.get())
          ->set_result(std::forward<Ts>(ts)...);
        this->promise_.reset();
    }

    std::unique_ptr<void, detail::fiber_context::resumer> promise_;
    Executor executor_;
};

template<class F, class Executor, class Exception = broken_promise>
struct fiber_main
{
    boost::context::fiber operator()(boost::context::fiber&& fiber)
    {
        fiber_context ctx{std::move(fiber)};
        BOOST_TRY
        {
            yield_token<Executor> token{std::move(executor_), ctx};
            boost::asio::post(token);
            f_(std::move(token));
        }
        BOOST_CATCH(Exception const&)
        {
            // Ignoring this exception allows an application to cleanup properly
            // if there are pending operations when
            // execution_context::shutdown() is called.
        }
        BOOST_CATCH_END
        return ctx.final_suspend();
    }

    F f_;
    Executor executor_;
};

} // namespace detail
} // namespace ufiber

namespace boost
{
namespace asio
{

template<class Executor, class... Args>
class async_result<::ufiber::yield_token<Executor>, void(Args...)>
{
public:
    using completion_handler_type =
      ::ufiber::detail::completion_handler<Executor, Args...>;

    using return_type = ::ufiber::detail::result_t<Args...>;

    template<class Op, class Token, class... Ts>
    static return_type initiate(Op&& op, Token&& token, Ts&&... ts)
    {
        ::ufiber::detail::promise<Args...> promise;
        ::ufiber::detail::fiber_context& ctx =
          ::ufiber::detail::get_fiber(token);
        completion_handler_type handler{&promise, token, ctx};
        ctx.suspend_with([&]() noexcept {
            op(std::move(handler), std::forward<Ts>(ts)...);
        });
        return promise.get_value();
    }

    return_type get() = delete;
};

template<class Executor, class E, class... Args>
class associated_executor<
  ::ufiber::detail::completion_handler<Executor, Args...>,
  E>
{
public:
    using type = Executor;

    static type get(
      ::ufiber::detail::completion_handler<Executor, Args...> const& h,
      E const& = E())
    {
        return h.executor_;
    }
};

} // asio

} // boost

#endif // UFIBER_DETAIL_UFIBER_HPP
