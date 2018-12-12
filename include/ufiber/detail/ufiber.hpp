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

class fiber_context
{
public:
    struct resumer
    {
        UFIBER_INLINE_DECL void operator()(void* promise) noexcept;
        fiber_context& ctx_;
        std::thread::id id_;
    };

    UFIBER_INLINE_DECL explicit fiber_context(boost::context::fiber&& f);
    fiber_context(fiber_context&&) = delete;
    fiber_context(fiber_context const&) = delete;

    fiber_context& operator=(fiber_context&&) = delete;
    fiber_context& operator=(fiber_context const&) = delete;

    UFIBER_INLINE_DECL void suspend() noexcept;

    UFIBER_INLINE_DECL void resume() noexcept;

    UFIBER_INLINE_DECL boost::context::fiber final_suspend() noexcept;

private:
    boost::context::fiber fiber_;
    std::atomic<bool> running_;
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

class completion_handler_base
{
public:
    UFIBER_INLINE_DECL completion_handler_base(
      detail::fiber_context& ctx) noexcept;

    UFIBER_INLINE_DECL void attach(void* promise) noexcept;

    UFIBER_INLINE_DECL fiber_context& get_context() const noexcept;

protected:
    std::unique_ptr<void, detail::fiber_context::resumer> promise_;
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
        ::new (static_cast<void*>(&result_))
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
                throw broken_promise{};
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
class completion_handler : public completion_handler_base
{
public:
    using executor_type = Executor;

    explicit completion_handler(yield_token<Executor>& yt)
      : completion_handler_base{detail::get_fiber(yt)}
      , executor_{yt.get_executor()}
    {
    }

    executor_type get_executor() const noexcept
    {
        return executor_;
    }

    template<class... Ts>
    void operator()(Ts&&... ts)
    {
        static_cast<promise<Args...>*>(this->promise_.get())
          ->set_result(std::forward<Ts>(ts)...);
        this->promise_.reset();
    }

private:
    Executor executor_;
};

template<class F, class Executor>
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
        BOOST_CATCH(broken_promise const&)
        {
            // Ignoring this exception allows an application to cleanup properly
            // if there are pending operations when
            // execution_context::shutdown() is called.
        }
        BOOST_CATCH_END

        return std::move(ctx.final_suspend());
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

    explicit async_result(completion_handler_type& cht)
      : ctx_{cht.get_context()}
    {
        cht.attach(&promise_);
    }

    return_type get()
    {
        ctx_.suspend(); // what if initiating the composed operation throws?
        return promise_.get_value();
    }

private:
    ::ufiber::detail::fiber_context& ctx_;
    ::ufiber::detail::promise<Args...> promise_;
};

} // asio

} // boost

#endif // UFIBER_DETAIL_UFIBER_HPP
