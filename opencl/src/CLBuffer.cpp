#include "CLBuffer.h"

#include "CLLoader.h"

namespace core {
namespace opencl {

Result<std::unique_ptr<CLBuffer>> CLBuffer::Create(CLContext* context,
                                                   const size_t buffer_size,
                                                   const cl_mem_flags flags,
                                                   void* host_ptr) {
  std::unique_ptr<CLBuffer> object(new CLBuffer());
  const Status status = object->Init(context, buffer_size, flags, host_ptr);
  if (!status.ok()) {
    return Result<std::unique_ptr<CLBuffer>>(status);
  }
  return Result<std::unique_ptr<CLBuffer>>(std::move(object));
}

Status CLBuffer::Init(CLContext* context, const size_t buffer_size,
                      const cl_mem_flags flags, void* host_ptr) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "CLBuffer initialization is single-use");
  }
  initialized_ = true;
  size = buffer_size;
  context_ = context;

  if (context_ == nullptr || context_->context == nullptr) {
    return Status(ErrorCode::kInvalidArgument,
                  "A live OpenCL context is required");
  }
  cl_int err = CL_SUCCESS;
  // clCreateBuffer's host_ptr is non-const; cast away constness as API does not modify data.
  buffer = clCreateBuffer(context_->context, flags, buffer_size, host_ptr, &err);
  if (err != CL_SUCCESS || !buffer) {
    return Status(ErrorCode::kBackend, "clCreateBuffer failed", "OpenCL", err);
  }

  return Status();
}

CLBuffer::~CLBuffer() {
  if (buffer != nullptr) {
    clReleaseMemObject(buffer);
    buffer = nullptr;
  }
}

}  // namespace opencl
}  // namespace core
