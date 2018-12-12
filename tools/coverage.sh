#! /bin/bash
lcov --directory build --capture --output-file build/coverage.info && \
lcov --extract build/coverage.info $(pwd)'/include/*' --output-file build/coverage.info && \
lcov --list build/coverage.info
