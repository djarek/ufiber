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
    // Check if destroying the io_context properly destroys the fibers if the
    // operations are abandoned.
    int count = 0;
    bool abandoned = false;
    std::function<void(yield_token_t)> suspension_op = [](yield_token_t yield) {
        boost::asio::post(yield);
    };
    auto f = [&](yield_token_t yield) {
        ++count;
        try
        {
            suspension_op(yield);
        }
        catch (ufiber::broken_promise const& ex)
        {
            BOOST_TEST(ex.what() == std::string{"Broken fiber promise"});
            abandoned = true;
            throw;
        }
        ++count;
    };

    {
        abandoned = false;
        count = 0;
        boost::asio::io_context io{};
        ufiber::spawn(io, f);

        BOOST_TEST(io.run_one() > 0);
    }
    BOOST_TEST(abandoned == true);
    BOOST_TEST(count == 1);

    {
        abandoned = false;
        count = 0;
        boost::asio::io_context io{};
        ufiber::spawn(io.get_executor(), f);

        BOOST_TEST(io.run_one() > 0);
    }
    BOOST_TEST(abandoned == true);
    BOOST_TEST(count == 1);

    {
        abandoned = false;
        count = 0;
        boost::asio::io_context io{};
        ufiber::spawn(std::allocator_arg,
                      boost::context::default_stack{},
                      io.get_executor(),
                      f);

        BOOST_TEST(io.run_one() > 0);
    }
    BOOST_TEST(abandoned == true);
    BOOST_TEST(count == 1);

    {
        abandoned = false;
        count = 0;
        boost::asio::io_context io{};
        suspension_op = [&](yield_token_t yield) {
            int n = ufiber::test::async_op_1arg(io, 42, yield);
            (void)n;
            BOOST_ERROR("Abandoned fiber has been resumed normally");
        };
        ufiber::spawn(io, f);

        BOOST_TEST(io.run_one() > 0);
    }
    BOOST_TEST(abandoned == true);
    BOOST_TEST(count == 1);

    {
        abandoned = false;
        count = 0;
        boost::asio::io_context io{};
        suspension_op = [&](yield_token_t yield) {
            int n;
            std::unique_ptr<int> p;

            std::tie(n, p) = ufiber::test::async_op_2arg(
              io, 42, boost::make_unique<int>(43), yield);
            BOOST_ERROR("Abandoned fiber has been resumed normally");
        };
        ufiber::spawn(io, f);

        BOOST_TEST(io.run_one() > 0);
    }
    BOOST_TEST(abandoned == true);
    BOOST_TEST(count == 1);

    return boost::report_errors();
}
