# μfiber

Language|Build | Coverage| License |
|-------|------|---------|---------|
|[![Standard](https://img.shields.io/badge/C%2B%2B-11-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization) | [![Build Status](https://dev.azure.com/damianjarek93/ufiber/_apis/build/status/djarek.ufiber?branchName=master)](https://dev.azure.com/damianjarek93/ufiber/_build/latest?definitionId=1?branchName=master) | [![codecov](https://codecov.io/gh/djarek/ufiber/branch/master/graph/badge.svg)](https://codecov.io/gh/djarek/ufiber) | [![License](https://img.shields.io/badge/license-BSL%201.0-blue.svg)](https://opensource.org/licenses/BSL-1.0)


## Introduction
**μfiber** is a minimalistic, header-only fiber library compatible with the
asynchronous operation model specified in the C++ Networking TS and implemented
in Boost.ASIO. It implements an interface similar to `boost::asio::spawn()`, but
takes advantage of C++11 move semantics to avoid additional memory allocations
that are necessary in `boost::asio::spawn()` and supports more than 2 results of
an asynchronous operation. Additionally, this library does not use deprecated
Boost libraries, so it generates no deprecation warnings when used.

## Dependencies
**μfiber** depends on:
- Boost.Context
- Boost.ASIO

## Installation
**μfiber** is header-only, so you only need to add the include directory to the
include paths in your build system. An `install` target is available in CMake
which will install the headers and a CMake `find_package` configuration script
for easy consumption in projects built with CMake:
```bash
mkdir build
cd build
cmake ..
make install
```

After installation, a project built with CMake can consume the library using
`find_package`:
```cmake
find_package(ufiber REQUIRED)
target_link_libraries(my_target PUBLIC ufiber::ufiber)
```

## Examples
- [Echo](https://github.com/djarek/ufiber/blob/master/examples/echo.cpp) - a
  well documented,  echo server that uses fibers to accept clients and then
  sends back octets to the client.


## API
### `yield_token`
```c++
template<class Executor>
class yield_token
{
public:
    using executor_type = Executor;

    executor_type get_executor() noexcept;
};

struct broken_promise final : std::exception
{
    char const* what() const noexcept final;
};

```
`yield_token` is a lightweight handle to the currently running fiber, which may
be used as a `CompletionToken` in an asynchronous initiation function. The
behavior of a `yield_token`, outside the fiber it was created on, is undefined.
`Executor` shall satisfy the constraints of
[Executor](https://www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/Executor1.html).
The executor on which the current fiber runs can be retrieved via
`get_executor()`.

When `yield_token` is used as a
[CompletionToken](https://www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/asynchronous_operations.html),
the asynchronous initiation function that it is passed to as an argument will
suspend execution of the current fiber. Once the operation completes, the fiber
is resumed and the async function will return the results of the operation as
its return value. The return type depends on the number of results:
- 0 results: returns `void`
- 1 result: returns `T`
- N results: returns `std::tuple<T...>`

--------------------------

### Exceptions
If an asynchronous operation has been abandoned, the fiber will be resumed but
the asynchronous initiation function will throw `ufiber::broken_promise`. This
happens if an `io_context` is destroyed with pending asynchronous operations. If
exceptions are disabled, the user will need to provide their own definition of
`[[noreturn]] ufiber::detail::throw_broken_promise()`. Note that, if an
operation has been abandoned, the fiber may not be running within the context of
its executor, so catching `ufiber::broken_promise` is not recommended.

Any exception (except for `ufiber::broken_promise`) that excapes the fiber's
main function, will result in a call to `std::terminate()` (same behavior as in
`std::thread`).

--------------------------

### Spawning fibers
Fibers are created by calling one of the provided overloads of the `spawn()`
function. Fibers are scheduled on the executor they are spawned on, migration
between threads is managed by the executor. Note that when using this library
with executors provided by ASIO, one needs to either avoid using TLS
variables or ensure that the executor cannot schedule the fiber on another
thread.

1) Spawn a fiber on `ex` with the function object `f` as its main function. The
    implementation will construct an object from `f` via `DECAY_COPY`. This function
    participates in overload resolution if and only if, `E` satisfies the
    constraints of
    [Executor](https://www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/Executor1.html):

    ```c++
    template<class E, class F>
    auto
    spawn(E const& ex, F&& f) -> /* SFINAE */ void;
    ```

    Requires:
    - `F` shall be invokable with the signature `void(yield_token<E>)`

    Example usage:
    ```c++
    boost::asio::io_context io;
    boost::asio::strand<decltype(io)::executor_type> strand{io.get_executor()};
    ufiber::spawn(
        strand,
        [](ufiber::yield_token<decltype(strand)> yield)
        {
            // The fiber will run on the strand
            boost::asio::steady_timer timer{yield.get_executor().context()};
            timer.expires_from_now(std::chrono::seconds{1});
            auto ec = timer.async_wait(yield);
            // Handle the timer expiration
        });
    ```

    --------------------------

2) Spawn a fiber on `ctx`'s executor with the function object `f` as its main
    function. The implementation will construct an object from `f` via `DECAY_COPY`.
    This function participates in overload resolution if and only if, `Ctx`
    satisfies the constraints of
    [ExecutionContext](https://www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/ExecutionContext.html):

    ```c++
    template<class Ctx, class F>
    auto
    spawn(Ctx& ctx, F&& f) -> /* SFINAE */ void;
    ```

    Requires:
    - `F` shall be invokable with the signature
    `void(yield_token<Ctx::executor_type>)`.

    Example usage:
    ```c++
    boost::asio::io_context io;

    ufiber::spawn(
        io,
        [](auto yield)
        {
            boost::asio::steady_timer timer{yield.get_executor().context()};
            timer.expires_from_now(std::chrono::seconds{1});
            auto ec = timer.async_wait(yield);
            // Handle the timer expiration
        });
    ```

    --------------------------

2) Spawn a fiber on `ex` with the function object `f` as its main
    function, `sa` will be used to allocate the fiber's stack. The implementation will construct an object from `f` via
    `DECAY_COPY`:

    ```c++
    template<class Alloc, class E, class F>
    void
    spawn(std::allocator_arg_t arg, StackAllocator&& sa, E const& ex, F&& f);
    ```

    Requires:
    - `E` shall satisfy the constraints of
    [Executor](https://www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/Executor1.html).
    - `F` shall be invokable with the signature `void(yield_token<E>)`.
    - `Alloc` shall satisfy the constraints of
    [StackAllocator](https://www.boost.org/doc/libs/1_69_0/libs/context/doc/html/context/stack.html).

    Example usage:
    ```c++
    boost::asio::io_context io;

    ufiber::spawn(
        std::allocator_arg,
        boost::context::protected_fixedsize{PTHREAD_STACK_MIN},
        io.get_executor(),
        [](auto yield)
        {
            boost::asio::signal_set set{yield.get_executor().context()};
            set.add(SIGINT);
            set.add(SIGHUP);
            auto [ec, sig_num] = set.async_wait(yield);
            // Handle the signal
        });
    ```

## License
Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
