#! /bin/bash

set -e
lcov --directory $1 --capture --output-file $1/coverage.info && \
lcov --extract $1/coverage.info $(pwd)'/include/*' --output-file $1/coverage.info && \
lcov --list $1/coverage.info
