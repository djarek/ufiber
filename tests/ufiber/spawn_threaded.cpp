//
// Copyright (c) 2018 Damian Jarek (damian dot jarek93 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/djarek/ufiber
//

#include <ufiber/ufiber.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/core/lightweight_test.hpp>
#include <future>

namespace ufiber
{

namespace test
{

void
try_invoke(std::function<void()>& f)
{
    if (f)
    {
        f();
    }
}

template<class E>
class executor : public E
{
public:
    executor(E const& e,
             std::function<void()>& pre_hook,
             std::function<void()>& post_hook)
      : E{e}
      , pre_hook_{pre_hook}
      , post_hook_{post_hook}
    {
    }

    template<class F, class A>
    void dispatch(F&& f, A const& a) const
    {
        try_invoke(pre_hook_);
        E const& self = *this;
        self.dispatch(std::forward<F>(f), a);
        try_invoke(post_hook_);
    }

    template<class F, class A>
    void post(F&& f, A const& a) const
    {
        try_invoke(pre_hook_);
        E const& self = *this;
        self.post(std::forward<F>(f), a);
        try_invoke(post_hook_);
    }

    template<class F, class A>
    void defer(F&& f, A const& a) const
    {
        try_invoke(pre_hook_);
        E const& self = *this;
        self.defer(std::forward<F>(f), a);
        try_invoke(post_hook_);
    }

private:
    std::function<void()>& pre_hook_;
    std::function<void()>& post_hook_;
};

template<class CompletionToken>
auto
async_run(boost::asio::io_context& ctx,
          std::promise<void>& promise,
          CompletionToken&& tok)
  -> BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, void())
{
    boost::asio::async_completion<CompletionToken, void()> init{tok};

    using handler_t = typename decltype(init)::completion_handler_type;

    struct op
    {
        using executor_type =
          boost::asio::associated_executor_t<handler_t,
                                             decltype(ctx.get_executor())>;

        executor_type get_executor() const noexcept
        {
            return boost::asio::get_associated_executor(handler_,
                                                        io_.get_executor());
        }

        void operator()()
        {
            promise_.set_value();
            handler_();
        }

        handler_t handler_;
        std::promise<void>& promise_;
        boost::asio::io_context& io_;
    };

    boost::asio::post(op{std::move(init.completion_handler), promise, ctx});

    return init.result.get();
}

} // namespace test
} // namespace ufiber

int
main()
{
    using yield_token_t = ufiber::yield_token<
      ufiber::test::executor<boost::asio::io_context::executor_type>>;
    using executor_t = yield_token_t::executor_type;

    {
        // Check if an operation, completing before the fiber suspends is
        // handled properly
        std::function<void()> pre_hook;
        std::function<void()> post_hook;
        std::promise<void> promise;
        boost::asio::io_context io{2};
        executor_t ex{io.get_executor(), pre_hook, post_hook};
        ufiber::spawn(ex, [&](yield_token_t yield) {
            auto wg = boost::asio::make_work_guard(io);
            // Emulate the completion of an operation before the fiber has
            // had a chance to suspend
            post_hook = [&]() {
                auto const status =
                  promise.get_future().wait_for(std::chrono::milliseconds{500});
                BOOST_TEST(status == std::future_status::ready);
            };

            ufiber::test::async_run(io, promise, yield);
        });
        std::thread t{[&io]() { BOOST_TEST(io.run() == 1); }};

        BOOST_TEST(io.run() == 1);
        t.join();
    }

    {
        // Check if failure to allocate in an async init function is handled
        // properly (no resumption, just let the exception bubble to the fiber)
        std::function<void()> pre_hook;
        std::function<void()> post_hook;
        boost::asio::io_context io{1};
        executor_t ex{io.get_executor(), pre_hook, post_hook};
        int counter = 0;
        ufiber::spawn(ex, [&](yield_token_t yield) {
            // Emulate an allocation failure when initiating an operation.
            pre_hook = []() { throw std::bad_alloc{}; };
            try
            {
                boost::asio::post(yield);
                BOOST_ERROR("Expected exception");
            }
            catch (std::bad_alloc const&)
            {
                ++counter;
                // Success
            }
        });

        BOOST_TEST(io.run() == 1);
        BOOST_TEST(counter == 1);
    }

    return boost::report_errors();
}
