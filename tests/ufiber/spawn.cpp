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
#include <boost/make_unique.hpp>

#include "common.hpp"

int
main()
{
    using yield_token_t =
      ufiber::yield_token<boost::asio::io_context::executor_type>;
    int count = 0;
    std::function<void(yield_token_t)> suspension_op = [](yield_token_t yield) {
        boost::asio::post(yield);
    };
    auto f = [&](yield_token_t yield) {
        ++count;
        // Check we don't get spawned into the system executor by accident
        BOOST_TEST(yield.get_executor().running_in_this_thread());
        suspension_op(yield);
        BOOST_TEST(yield.get_executor().running_in_this_thread());
        ++count;
    };

    {
        count = 0;
        boost::asio::io_context io{};
        ufiber::spawn(io, f);

        BOOST_TEST(io.run() > 0);
    }
    BOOST_TEST(count == 2);

    {
        count = 0;
        boost::asio::io_context io{};
        ufiber::spawn(io.get_executor(), f);

        BOOST_TEST(io.run() > 0);
    }
    BOOST_TEST(count == 2);

    {
        count = 0;
        boost::asio::io_context io{};
        ufiber::spawn(std::allocator_arg,
                      boost::context::default_stack{},
                      io.get_executor(),
                      f);

        BOOST_TEST(io.run() > 0);
    }
    BOOST_TEST(count == 2);

    {
        // Check if a single argument is returned without wrapping into tuple
        count = 0;
        boost::asio::io_context io{};
        suspension_op = [&](yield_token_t yield) {
            int n = ufiber::test::async_op_1arg(io, 42, yield);
            BOOST_TEST(n == 42);
        };
        ufiber::spawn(io, f);

        BOOST_TEST(io.run() > 0);
    }
    BOOST_TEST(count == 2);

    {
        // Check if a 2 arguments are wrapped into a tuple and that non-trivial
        // objects are handled properly
        count = 0;
        boost::asio::io_context io{};
        suspension_op = [&](yield_token_t yield) {
            int n;
            std::unique_ptr<int> p;

            std::tie(n, p) = ufiber::test::async_op_2arg(
              io, 42, boost::make_unique<int>(43), yield);
            BOOST_TEST(n == 42);
            BOOST_TEST(p != nullptr && *p == 43);
        };
        ufiber::spawn(io, f);

        BOOST_TEST(io.run() > 0);
    }
    BOOST_TEST(count == 2);

    return boost::report_errors();
}
