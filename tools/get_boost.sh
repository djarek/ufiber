#! /bin/bash
trap 'exit $?' ERR
curl -L https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz -o boost.tar.gz
tar -xf boost.tar.gz
cd boost_*
./bootstrap.sh

B2_VARIANT=$(echo $CMAKE_VARIANT | awk '{print tolower($0)}')

sudo ./b2 --with-system --with-atomic --with-thread --with-date_time --with-chrono --with-context cxxstd=11 variant=$B2_VARIANT toolset=$CC install
cd -
