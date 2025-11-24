#include "CLBuffer.h"

#include "CLLoader.h"

namespace core {
namespace opencl {

CLBuffer::CLBuffer(CLContext* context, size_t size, cl_mem_flags flags, const void* host_ptr)
    : size(size), context_(context) {
  cl_int err = CL_SUCCESS;
  // clCreateBuffer's host_ptr is non-const; cast away constness as API does not modify data.
  buffer = clCreateBuffer(context_->context, flags, size, const_cast<void*>(host_ptr), &err);
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
