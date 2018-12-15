# μfiber, a fiber library in C++11, compatible with Boost.ASIO


| Build | Coverage
|-------|---------
| [![Build Status](https://dev.azure.com/damianjarek93/ufiber/_apis/build/status/djarek.ufiber?branchName=master)](https://dev.azure.com/damianjarek93/ufiber/_build/latest?definitionId=1?branchName=master)        | [![codecov](https://codecov.io/gh/djarek/ufiber/branch/master/graph/badge.svg)](https://codecov.io/gh/djarek/ufiber)


## Introduction
μfiber is a minimalistic, header-only fiber library compatible with the
asynchronous operation model specified in the C++ Networking TS and implemented
in Boost.ASIO. It implements an interface simillar to `boost::asio::spawn()`,
but takes advantage of C++11 move semantics to avoid additional memory
allocations that are necessary in `boost::asio::spawn()` and supports more than
2 results of an asynchronous operation. Additionally, this library does not use
deprecated Boost libraries, so it generates no deprecation warnings when used.

## Dependencies
μfiber depends on:
- Boost.Context
- Boost.ASIO

## Installation
μfiber is header-only, so you only need to add the include directory to the
include paths in your build system. An `install` target is available in CMake
which will install the headers and a CMake `find_package` configuration script
for easy consumption in projects built with CMake:
```
mkdir build
cd build
cmake ..
make install
```

## Examples

## License
Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
