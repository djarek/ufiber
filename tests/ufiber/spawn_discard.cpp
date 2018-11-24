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

int
main()
{
    // Check if destroying the io_context properly destroys the fibers.
    int count = 0;
    bool cleaned = false;
    auto f =
      [&](ufiber::yield_token<boost::asio::io_context::executor_type> yield) {
          ++count;
          try
          {
              boost::asio::post(yield);
          }
          catch (ufiber::broken_promise const&)
          {
              cleaned = true;
              return;
          }
          ++count;
      };

    {
        cleaned = false;
        count = 0;
        boost::asio::io_context io{};
        ufiber::spawn(io, f);

        BOOST_TEST(io.run_one() > 0);
    }
    BOOST_TEST(cleaned == true);
    BOOST_TEST(count == 1);

    {
        cleaned = false;
        count = 0;
        boost::asio::io_context io{};
        ufiber::spawn(io.get_executor(), f);

        BOOST_TEST(io.run_one() > 0);
    }
    BOOST_TEST(cleaned == true);
    BOOST_TEST(count == 1);

    {
        cleaned = false;
        count = 0;
        boost::asio::io_context io{};
        ufiber::spawn(std::allocator_arg,
                      boost::context::default_stack{},
                      io.get_executor(),
                      f);

        BOOST_TEST(io.run_one() > 0);
    }
    BOOST_TEST(cleaned == true);
    BOOST_TEST(count == 1);

    return boost::report_errors();
}
