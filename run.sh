#! /bin/bash
set -x -e

mkdir -p build
cd build

cmake ..
make

cd ..

# Run unit test
# ./build/tests/core-unit-test

# Run Vulkan tests
./build/vulkan/tests/vulkan_tests