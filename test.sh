#!/usr/bin/env bash

BUILD_TYPE=${1:-Release}

set -euo pipefail

mkdir -p build
cd build
mkdir -p out
cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=$(pwd)/out
make -j$(nproc)
make test ARGS="--output-on-failure"
