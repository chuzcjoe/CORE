cmake_minimum_required(VERSION 3.10)
project(CORE)

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

# Core module build options. All modules are enabled by default and can be
# disabled from the CMake command line, for example: -DENABLE_VULKAN=OFF.
option(ENABLE_EXTERNAL "Build external dependencies" ON)
option(ENABLE_VULKAN "Build the Vulkan module" ON)
option(ENABLE_OPENCL "Build the OpenCL module" ON)
option(ENABLE_MAT "Build the math module" ON)
option(ENABLE_TIMER "Build the timer module" ON)
option(ENABLE_TRACE "Build the tracing module" ON)
option(ENABLE_OPENGL "Build the OpenGL module" ON)
option(ENABLE_METAL "Build the Metal module" ON)
option(ENABLE_EGL "Build the EGL module" ON)
option(ENABLE_IO "Build the IO module" ON)
option(ENABLE_THREADPOOL "Build the thread pool module" ON)
option(ENABLE_TESTS "Build core tests" ON)
option(ENABLE_EXAMPLES "Build examples" ON)

find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan: ${Vulkan_LIBRARY}, ${Vulkan_INCLUDE_DIRS}")

# Set pipeline cache
message(STATUS "Using pipeline cache dir: ${PIPELINE_CACHE_DIR}")
add_definitions(-DPIPELINE_CACHE_DIR="${PIPELINE_CACHE_DIR}")

# cmake
include("${CORE_SOURCE_DIR}/cmake/example.cmake")
include("${CORE_SOURCE_DIR}/cmake/${BUILD_VARIANT}.cmake")
