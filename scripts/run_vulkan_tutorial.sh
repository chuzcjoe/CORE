#! /bin/bash
set -e
set -x

# save tmp data
rm -rf ./tmp
mkdir -p ./tmp

rm -rf build/vulkan_tutorial
mkdir -p build/vulkan_tutorial
cd build/vulkan_tutorial

cmake_options=(-DCMAKE_BUILD_TYPE=Debug
                -DBUILD_VARIANT=vulkan_tutorial)

cmake "${cmake_options[@]}" ../..
make -j10

cd ../..
