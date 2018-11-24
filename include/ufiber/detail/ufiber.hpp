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
#include <ufiber/detail/monotonic_allocator.hpp>

#include <boost/asio/post.hpp>
#include <boost/context/fiber.hpp>

#include <atomic>

namespace ufiber
{

template<class Executor, std::size_t slab_size = 1024>
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
        fiber_context& ctx;
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

template<class Executor, std::size_t slab_size>
detail::fiber_context&
get_fiber(yield_token<Executor, slab_size>& yt)
{
    return yt.ctx_;
}

UFIBER_INLINE_DECL void
initial_resume(boost::context::fiber&& f);

enum class result_type
{
    broken_promise,
    value,
};

class completion_handler_base
{
public:
    using allocator_type = monotonic_allocator<void>;

    UFIBER_INLINE_DECL completion_handler_base(
      detail::fiber_context& ctx) noexcept;

    UFIBER_INLINE_DECL allocator_type get_allocator() const noexcept;

    UFIBER_INLINE_DECL void attach(void* promise,
                                   memory_resource_base& mr) noexcept;

    UFIBER_INLINE_DECL fiber_context& get_context() const noexcept;

protected:
    std::unique_ptr<void, detail::fiber_context::resumer> promise_;
    allocator_type allocator_;
};

template<class... Args>
struct promise
{
    promise() = default;

    ~promise()
    {
        switch (type_)
        {
            case result_type::broken_promise:
                break;
            case result_type::value:
                result_.~tuple();
                break;
        }
    }

    template<class... Ts>
    void set_result(Ts&&... ts)
    {
        ::new (static_cast<void*>(&result_))
          std::tuple<Args...>{std::forward<Ts>(ts)...};
        type_ = result_type::value;
    }

    std::tuple<Args...> get_value()
    {
        switch (type_)
        {
            case result_type::value:
                return std::move(result_);
            default:
                throw broken_promise{};
        }
    }

private:
    result_type type_ = result_type::broken_promise;
    union {
        std::tuple<Args...> result_;
    };
};

template<class Executor, class... Args>
class completion_handler : public completion_handler_base
{
public:
    using executor_type = Executor;

    template<std::size_t slab_size>
    explicit completion_handler(yield_token<Executor, slab_size>& yt)
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

template<class F, class Executor, std::size_t slab_size = 1024>
struct fiber_main
{
    boost::context::fiber operator()(boost::context::fiber&& fiber)
    {
        fiber_context ctx{std::move(fiber)};
        try
        {
            // Get on the right executor. Posting here, rather than in spawn()
            // allows us to use the slab allocator.
            yield_token<Executor, slab_size> token{std::move(executor), ctx};
            boost::asio::post(token);
            f(std::move(token));
        }
        catch (broken_promise const&)
        {
            // Ignoring this exception allows an application to cleanup properly
            // if there are pending operations when the
            // execution_context::shutdown() is called.
        }

        return std::move(ctx.final_suspend());
    }

    F f;
    Executor executor;
};

} // namespace detail
} // namespace ufiber

namespace boost
{
namespace asio
{

template<class Executor, std::size_t slab_size, class... Args>
class async_result<::ufiber::yield_token<Executor, slab_size>, void(Args...)>
{
public:
    using completion_handler_type =
      ::ufiber::detail::completion_handler<Executor, Args...>;

    using return_type = std::tuple<Args...>;

    explicit async_result(completion_handler_type& cht)
      : ctx_{cht.get_context()}
    {
        cht.attach(&promise_, memory_);
    }

    return_type get()
    {
        ctx_.suspend(); // what if initiating the composed operation throws?
        return promise_.get_value();
    }

private:
    ::ufiber::detail::monotonic_memory_resource<slab_size> memory_;
    ::ufiber::detail::fiber_context& ctx_;
    ::ufiber::detail::promise<Args...> promise_;
};

} // asio

} // boost

#endif // UFIBER_DETAIL_UFIBER_HPP
