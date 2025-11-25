#pragma once

#include <stdexcept>
#include <type_traits>

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

  // Variadic setter: sets arguments sequentially starting from index 0
  // Enabled only when the first argument is not an integral type to avoid
  // clashing with the indexed overloads below.
  template <typename First, typename... Rest,
            typename = std::enable_if_t<!std::is_integral_v<std::decay_t<First>>>>
  void SetArgs(const First& first, const Rest&... rest) {
    SetArgsImpl(0, first, rest...);
  }

 private:
  // Base case for recursion
  void SetArgsImpl(cl_uint) {}

  // Overload for CLBuffer
  void SetArgSingle(cl_uint index, const CLBuffer& buf) {
    cl_mem mem = buf.buffer;
    cl_int err = clSetKernelArg(kernel, index, sizeof(cl_mem), &mem);
    if (err != CL_SUCCESS) throw std::runtime_error("clSetKernelArg failed (CLBuffer)");
  }

  // Generic trivially copyable scalar types (non-pointers)
  template <typename T>
  std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>> SetArgSingle(
      cl_uint index, const T& value) {
    cl_int err = clSetKernelArg(kernel, index, sizeof(T), &value);
    if (err != CL_SUCCESS) throw std::runtime_error("clSetKernelArg failed");
  }

  template <typename First, typename... Rest>
  void SetArgsImpl(cl_uint index, const First& first, const Rest&... rest) {
    SetArgSingle(index, first);
    SetArgsImpl(index + 1, rest...);
  }
};

}  // namespace opencl
}  // namespace core
