#pragma once

#include <stdexcept>

#include "CLBuffer.h"
#include "CLContext.h"
#include "CLProgram.h"

namespace core {
namespace opencl {

class CLKernel {
 public:
  // Create a kernel from a built program and kernel name.
  CLKernel(CLProgram* program, const char* kernel_name);
  ~CLKernel();

  cl_kernel kernel = nullptr;

  void SetArgs(cl_uint index, const CLBuffer& buf) {
    cl_mem mem = buf.buffer;
    cl_int err = clSetKernelArg(kernel, index, sizeof(cl_mem), &mem);
    if (err != CL_SUCCESS) throw std::runtime_error("clSetKernelArg failed (CLBuffer)");
  }
};

}  // namespace opencl
}  // namespace core
