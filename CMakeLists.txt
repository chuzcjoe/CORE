cmake_minimum_required(VERSION 3.10)
project(CORE)

# Make the C++20 requirement below authoritative for CMake's own standard
# resolution, not just a raw string in CMAKE_CXX_FLAGS. Without this,
# dependencies that declare their own minimum via target_compile_features()
# (e.g. googletest's `target_compile_features(gtest PUBLIC cxx_std_17)`) are
# the only thing CMake's std-selection logic knows about, so it can silently
# append a lower -std= flag *after* the one in CMAKE_CXX_FLAGS -- and since
# compilers honor the last -std= flag on the command line, that quietly
# downgrades the effective language mode.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Release)
if (BUILD_VARIANT STREQUAL core)
  set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -Wextra -Wpedantic -Wunused-variable -Wunused-function -Wshadow -Werror -O3")
elseif (BUILD_VARIANT STREQUAL vulkan_tutorial)
  set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -Wextra -Wpedantic -Wunused-variable -Wunused-function -O3")
endif()
set(ANDROID_STL "c++_shared")

find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan: ${Vulkan_LIBRARY}, ${Vulkan_INCLUDE_DIRS}")

# Set pipeline cache
message(STATUS "Using pipeline cache dir: ${PIPELINE_CACHE_DIR}")
add_definitions(-DPIPELINE_CACHE_DIR="${PIPELINE_CACHE_DIR}")

# cmake
include("${CORE_SOURCE_DIR}/cmake/example.cmake")
include("${CORE_SOURCE_DIR}/cmake/${BUILD_VARIANT}.cmake")
