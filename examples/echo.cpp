#include <ufiber/ufiber.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <iostream>

namespace echo
{
namespace net = boost::asio;

using tcp_socket =
  net::basic_stream_socket<net::ip::tcp, net::io_context::executor_type>;

using tcp_acceptor =
  net::basic_socket_acceptor<net::ip::tcp, net::io_context::executor_type>;

namespace
{
// We use a function object here so that we can easily bind together the socket
// and a function that operates on it. We could use `std::bind` instead, but
// that uses a lot of template machinery for the same effect.
struct echo_session
{
    void operator()(ufiber::yield_token<net::io_context::executor_type> yield)
    {
        // Make use of the fiber's stack by placing a static buffer on it.
        std::uint8_t buffer[8192];
        for (;;)
        {
            boost::system::error_code ec;
            std::size_t n;
            // We can make use of `std::tie` to conveniently assign the results
            // of an async operation into stack variables. In C++17, structured
            // bindings can be used instead.
            std::tie(ec, n) =
              socket_.async_read_some(boost::asio::buffer(buffer), yield);
            if (ec)
            {
                std::cerr << "Error while reading from socket: " << ec.message()
                          << '\n';
                return;
            }

            std::tie(ec, n) =
              boost::asio::async_write(socket_, boost::asio::buffer(buffer, n), yield);
            if (ec)
            {
                std::cerr << "Error while writing to socket: " << ec.message()
                          << '\n';
                return;
            }
        }
    }

    tcp_socket socket_;
};

// μfiber uses a yield_token that has an Executor parameter, so no type-erasure
// is used. The only allocations that will be performed when using μfiber and
// boost::asio are the alocations of the fiber stacks and temporary allocations
// done by ASIO for async operations. The latter are recycled internally by
// ASIO, so in practice you'll see very few allocations that actually touch the
// global heap.
void
accept(ufiber::yield_token<net::io_context::executor_type> yield)
{
    auto ex = yield.get_executor();
    tcp_acceptor acceptor{
      ex.context(), net::ip::tcp::endpoint{net::ip::address_v6::any(), 8000}};

    tcp_socket s{ex.context()};
    for (;;)
    {
        // When an async operation returns 1 result, it's not wrapped into a
        // tuple, so we can just use it directly.
        boost::system::error_code ec = acceptor.async_accept(s, yield);
        if (ec)
        {
            std::cerr << "Error when trying to accept: " << ec.message()
                      << '\n';
            return;
        }
        // Spawn a new fiber, use a function object as its entry point. Note
        // that the entry point is not copyable, but is move constructible. Here
        // we use the overload which accepts an Executor, rather than an
        // ExecutionContext.
        ufiber::spawn(ex, echo_session{std::move(s)});
        // Note that a moved-from socket is guaranteed to be in a state usable
        // for `async_accept`, so we can hoist construction of `s` out of the
        // loop, because the "default" constructor of `socket` is fairly
        // expensive (requires searching for services in the `io_context`'s
        // service registry).
    }
}

void
run()
{
    net::io_context io;
    // Spawn a new fiber, we use a function pointer as its entry point. The
    // newly created fiber will run on the provided ExecutionContext
    // and the current thread of execution is not blocked.
    ufiber::spawn(io, &echo::accept);
    // The spawned fiber will not run until there is a thread that "runs" within
    // the `io_context`.
    io.run();
}

} // namespace
} // namespace echo

int
main()
{
    echo::run();
}
