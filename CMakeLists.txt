cmake_minimum_required(VERSION 3.10)
project(CORE)

set(CMAKE_BUILD_TYPE Release)
if (BUILD_VARIANT STREQUAL core)
  set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -Wextra -Wpedantic -Wunused-variable -Wunused-function -Wshadow -Werror -O3")
elseif (BUILD_VARIANT STREQUAL vulkan_tutorial)
  set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -Wextra -Wpedantic -Wunused-variable -Wunused-function -O3")
endif()
set(ANDROID_STL "c++_shared")

# Inject Vulkan paths from environment (CI-friendly)
if(DEFINED ENV{Vulkan_INCLUDE_DIR} AND DEFINED ENV{Vulkan_LIBRARY})
  set(Vulkan_INCLUDE_DIR $ENV{Vulkan_INCLUDE_DIR})
  set(Vulkan_LIBRARY $ENV{Vulkan_LIBRARY})
endif()
message(STATUS "Vulkan_INCLUDE_DIR=${Vulkan_INCLUDE_DIR}")
message(STATUS "Vulkan_LIBRARY=${Vulkan_LIBRARY}")

find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan: ${Vulkan_LIBRARY}")

# Set pipeline cache
message(STATUS "Using pipeline cache dir: ${PIPELINE_CACHE_DIR}")
add_definitions(-DPIPELINE_CACHE_DIR="${PIPELINE_CACHE_DIR}")

# cmake
include(cmake/example.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/${BUILD_VARIANT}.cmake)
