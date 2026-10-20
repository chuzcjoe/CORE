#pragma once

#include <memory>
#include <type_traits>

#include "CLBuffer.h"
#include "CLContext.h"
#include "CLProgram.h"
#include "Status.h"

namespace core {
namespace opencl {

class CLKernel {
 public:
  CLKernel(const CLKernel&) = delete;
  CLKernel& operator=(const CLKernel&) = delete;
  CLKernel() = default;
  // Create a kernel from a built program and kernel name.

  static Result<std::unique_ptr<CLKernel>> Create(CLProgram* program,
                                                  const char* kernel_name);
  Status Init(CLProgram* program, const char* kernel_name);
  ~CLKernel();

  cl_kernel kernel = nullptr;

  // Variadic setter: sets arguments sequentially starting from index 0
  // Enabled only when the first argument is not an integral type to avoid
  // clashing with the indexed overloads below.
  template <
      typename First, typename... Rest,
      typename = std::enable_if_t<!std::is_integral_v<std::decay_t<First>>>>
  Status SetArgs(const First& first, const Rest&... rest) {
    return SetArgsImpl(0, first, rest...);
  }

 private:
  // Base case for recursion
  Status SetArgsImpl(cl_uint) { return Status(); }

  // Overload for CLBuffer
  Status SetArgSingle(cl_uint index, const CLBuffer& buf) {
    cl_mem mem = buf.buffer;
    cl_int err = clSetKernelArg(kernel, index, sizeof(cl_mem), &mem);
    if (err != CL_SUCCESS) {
      return Status(ErrorCode::kBackend, "clSetKernelArg failed (CLBuffer)",
                    "OpenCL", err);
    }
    return Status();
  }

  // Generic trivially copyable scalar types (non-pointers)
  template <typename T>
  std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>,
                   Status>
  SetArgSingle(cl_uint index, const T& value) {
    cl_int err = clSetKernelArg(kernel, index, sizeof(T), &value);
    if (err != CL_SUCCESS) {
      return Status(ErrorCode::kBackend, "clSetKernelArg failed", "OpenCL",
                    err);
    }
    return Status();
  }

  template <typename First, typename... Rest>
  Status SetArgsImpl(cl_uint index, const First& first, const Rest&... rest) {
    const Status status = SetArgSingle(index, first);
    if (!status.ok()) {
      return status;
    }
    return SetArgsImpl(index + 1, rest...);
  }

 private:
  bool initialized_ = false;
};

}  // namespace opencl
}  // namespace core
