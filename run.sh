#! /bin/bash
set -x -e

mkdir -p build
cd build

cmake ..
make

cd ..

# Run unit test
./build/test/unit-test