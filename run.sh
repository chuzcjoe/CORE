#! /bin/bash
set -x -e

echo ".run.sh (arm64-v8a/macos) (test target)"

target=$1
test_target=$2

mkdir -p build
cd build

cmake ..
make

cd ..

if [ "$test_target" = "vulkan" ]; then
    # Run Vulkan tests
    echo "run vulkan tests"
    ./build/vulkan/tests/vulkan_tests
fi