#include <gtest/gtest.h>

#include <iostream>

#include "CLLoader.h"

namespace core {
namespace test {

TEST(OpenCL, test1) {
  int init = core::opencl::cl_init();
  if (init) {
    throw std::runtime_error("Failed to initialize OpenCL loader");
  }

  cl_int error = 0;
  cl_platform_id platform_ids[10];
  cl_uint num_platforms;
  error = clGetPlatformIDs(10, platform_ids, &num_platforms);
  if (error != CL_SUCCESS) {
    throw std::runtime_error("clGetPlatformIDs failed");
  }
  std::cout << "num platforms: " << num_platforms << std::endl;
}

}  // namespace test
}  // namespace core