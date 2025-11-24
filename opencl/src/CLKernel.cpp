#include "CLKernel.h"

#include <stdexcept>

#include "CLLoader.h"

namespace core {
namespace opencl {

CLKernel::CLKernel(CLProgram* program, const char* kernel_name) {
  cl_int err = CL_SUCCESS;
  kernel = clCreateKernel(program->program, kernel_name, &err);
  if (err != CL_SUCCESS || !kernel) {
    throw std::runtime_error("clCreateKernel failed");
  }
}

CLKernel::~CLKernel() {
  if (kernel != nullptr) {
    clReleaseKernel(kernel);
    kernel = nullptr;
  }
}

}  // namespace opencl
}  // namespace core
