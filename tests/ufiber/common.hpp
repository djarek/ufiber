//
// Copyright (c) 2018 Damian Jarek (damian dot jarek93 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/djarek/ufiber
//

#ifndef UFIBER_TEST_COMMON
#define UFIBER_TEST_COMMON

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace ufiber
{
namespace test
{
template<class CompletionToken>
auto
async_op_1arg(boost::asio::io_context& ctx, int val, CompletionToken&& tok)
  -> BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, void(int))
{
    using handler_t = BOOST_ASIO_HANDLER_TYPE(CompletionToken, void(int));
    struct op
    {
        void operator()()
        {
            handler_(val_);
        }

        handler_t handler_;
        int val_;
    };

    return boost::asio::async_initiate<CompletionToken, void(int)>(
      [&](handler_t&& handler) {
          auto ex =
            boost::asio::get_associated_executor(handler, ctx.get_executor());
          boost::asio::post(ex, op{std::move(handler), val});
      },
      std::forward<CompletionToken>(tok));
}

template<class CompletionToken>
auto
async_op_2arg(boost::asio::io_context& ctx,
              int val1,
              std::unique_ptr<int> val2,
              CompletionToken&& tok)
  -> BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken,
                                   void(int, std::unique_ptr<int>))
{
    using handler_t =
      BOOST_ASIO_HANDLER_TYPE(CompletionToken, void(int, std::unique_ptr<int>));
    struct op
    {
        void operator()()
        {
            handler_(val1_, std::move(val2_));
        }

        handler_t handler_;
        int val1_;
        std::unique_ptr<int> val2_;
    };

    return boost::asio::async_initiate<CompletionToken,
                                       void(int, std::unique_ptr<int>)>(
      [&](handler_t&& handler) {
          auto ex =
            boost::asio::get_associated_executor(handler, ctx.get_executor());
          boost::asio::post(ex, op{std::move(handler), val1, std::move(val2)});
      },
      std::forward<CompletionToken>(tok));
}

} // namespace test
} // namespace ufiber

#endif // UFIBER_TEST_COMMON
