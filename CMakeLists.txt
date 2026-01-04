cmake_minimum_required(VERSION 3.10)
project(CORE)

set(CMAKE_BUILD_TYPE Release)
if (BUILD_VARIANT STREQUAL core)
  set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -Wextra -Wpedantic -Wunused-variable -Wunused-function -Wshadow -Werror -O3")
elseif (BUILD_VARIANT STREQUAL vulkan_tutorial)
  set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -Wextra -Wpedantic -Wunused-variable -Wunused-function -O3")
endif()
set(ANDROID_STL "c++_shared")

find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan: ${Vulkan_LIBRARY}")

# cmake
include(cmake/example.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/${BUILD_VARIANT}.cmake)
