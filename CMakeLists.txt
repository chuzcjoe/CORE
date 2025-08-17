cmake_minimum_required(VERSION 3.10)
project(CORE)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -Wextra -Wpedantic -Wunused-variable -Wunused-function -Wshadow -Werror -O3")
set(ANDROID_STL "c++_shared")

add_subdirectory(gtest)
add_subdirectory(tests)