#pragma once

#include <cstddef>
#include <stdexcept>

#include "CLContext.h"

namespace core {
namespace opencl {

class CLBuffer {
 public:
  // Create a buffer of given size and flags. Optionally initialize from host_ptr.
  // Throws std::runtime_error on failure.
  CLBuffer(CLContext* context, size_t size, cl_mem_flags flags = CL_MEM_READ_WRITE,
           const void* host_ptr = nullptr);
  ~CLBuffer();

  cl_mem buffer = nullptr;
  size_t size = 0;

 private:
  CLContext* context_ = nullptr;
};

}  // namespace opencl
}  // namespace core
