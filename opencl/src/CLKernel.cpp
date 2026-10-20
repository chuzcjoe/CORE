#include "CLKernel.h"


#include "CLLoader.h"

namespace core {
namespace opencl {

Result<std::unique_ptr<CLKernel>> CLKernel::Create(CLProgram* program,
                                                   const char* kernel_name) {
  std::unique_ptr<CLKernel> object(new CLKernel());
  const Status status = object->Init(program, kernel_name);
  if (!status.ok()) {
    return Result<std::unique_ptr<CLKernel>>(status);
  }
  return Result<std::unique_ptr<CLKernel>>(std::move(object));
}

Status CLKernel::Init(CLProgram* program, const char* kernel_name) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "CLKernel initialization is single-use");
  }
  initialized_ = true;

  if (program == nullptr || program->program == nullptr ||
      kernel_name == nullptr) {
    return Status(ErrorCode::kInvalidArgument,
                  "A built program and kernel name are required");
  }
  cl_int err = CL_SUCCESS;
  kernel = clCreateKernel(program->program, kernel_name, &err);
  if (err != CL_SUCCESS || !kernel) {
    return Status(ErrorCode::kBackend, "clCreateKernel failed", "OpenCL", err);
  }

  return Status();
}

CLKernel::~CLKernel() {
  if (kernel != nullptr) {
    clReleaseKernel(kernel);
    kernel = nullptr;
  }
}

}  // namespace opencl
}  // namespace core
