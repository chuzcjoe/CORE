cmake_minimum_required(VERSION 3.10)
project(CORE)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -Wextra -Wpedantic -Wunused-variable -Wunused-function -Wshadow -Werror -O3")
set(ANDROID_STL "c++_shared")

find_package(Vulkan REQUIRED)

# cmake
include(cmake/example.cmake)

add_subdirectory(external)
add_subdirectory(vulkan)
add_subdirectory(opengl)
add_subdirectory(metal)
add_subdirectory(mat)
add_subdirectory(timer)
add_subdirectory(threadpool)
add_subdirectory(tests)
add_subdirectory(examples)