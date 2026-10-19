#!/usr/bin/env bash

BUILD_TYPE=${1:-Debug}

set -euo pipefail

mkdir -p asan
cd asan
mkdir -p out
cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=$(pwd)/out -DENABLE_ASAN=ON
make -j$(nproc)
make test ARGS="--output-on-failure"
cd ..

mkdir -p ubsan
cd ubsan
mkdir -p out
cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=$(pwd)/out -DENABLE_UBSAN=ON
make -j$(nproc)
make test ARGS="--output-on-failure"
cd ..


mkdir -p tsan
cd tsan
mkdir -p out
cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=$(pwd)/out -DENABLE_TSAN=ON
make -j$(nproc)
make test ARGS="--output-on-failure"
cd ..
