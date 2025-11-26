#include "CLBuffer.h"

#include "CLLoader.h"

namespace core {
namespace opencl {

CLBuffer::CLBuffer(CLContext* context, const size_t size, const cl_mem_flags flags, void* host_ptr)
    : size(size), context_(context) {
  cl_int err = CL_SUCCESS;
  // clCreateBuffer's host_ptr is non-const; cast away constness as API does not modify data.
  buffer = clCreateBuffer(context_->context, flags, size, host_ptr, &err);
  if (err != CL_SUCCESS || !buffer) {
    throw std::runtime_error("clCreateBuffer failed");
  }
}

CLBuffer::~CLBuffer() {
  if (buffer != nullptr) {
    clReleaseMemObject(buffer);
    buffer = nullptr;
  }
}

}  // namespace opencl
}  // namespace core
