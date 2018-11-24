//
// Copyright (c) 2018 Damian Jarek (damian dot jarek93 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/djarek/ufiber
//

#include <ufiber/ufiber.hpp>

#include <boost/asio/executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/core/lightweight_test.hpp>

int
main()
{
    // Check if destroying the io_context properly destroys the fibers.
    int count = 0;
    auto f = [&](
               ufiber::yield_token<boost::asio::io_context::executor_type> y) {
        ++count;
        ufiber::yield_token<boost::asio::io_context::executor_type, 0> yield{y};
        // Check we don't get spawned into the system executor by accident
        BOOST_TEST(yield.get_executor().running_in_this_thread());
        boost::asio::post(yield);
        BOOST_TEST(yield.get_executor().running_in_this_thread());
        ++count;
        ufiber::yield_token<boost::asio::executor, 256> yield2{y};
        boost::asio::post(yield2);
        BOOST_TEST(yield.get_executor().running_in_this_thread());
        ++count;
    };

    count = 0;
    boost::asio::io_context io{};
    ufiber::spawn(io, f);

    BOOST_TEST(io.run() > 0);
    BOOST_TEST(count == 3);

    return boost::report_errors();
}
