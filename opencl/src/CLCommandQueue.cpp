#include "CLCommandQueue.h"


#include "CLLoader.h"

namespace core {
namespace opencl {

Result<std::unique_ptr<CLCommandQueue>> CLCommandQueue::Create(
    CLContext* context, const cl_command_queue_properties properties) {
  std::unique_ptr<CLCommandQueue> object(new CLCommandQueue());
  const Status status = object->Init(context, properties);
  if (!status.ok()) {
    return Result<std::unique_ptr<CLCommandQueue>>(status);
  }
  return Result<std::unique_ptr<CLCommandQueue>>(std::move(object));
}

Status CLCommandQueue::Init(CLContext* context,
                            const cl_command_queue_properties properties) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "CLCommandQueue initialization is single-use");
  }
  initialized_ = true;
  context_ = context;

  if (context_ == nullptr || context_->context == nullptr) {
    return Status(ErrorCode::kInvalidArgument,
                  "A live OpenCL context is required");
  }
  cl_int err = CL_SUCCESS;
  queue = clCreateCommandQueue(context_->context, context_->device, properties, &err);
  if (err != CL_SUCCESS || !queue) {
    return Status(ErrorCode::kBackend, "clCreateCommandQueue failed", "OpenCL",
                  err);
  }

  return Status();
}

CLCommandQueue::~CLCommandQueue() {
  if (queue != nullptr) {
    clReleaseCommandQueue(queue);
    queue = nullptr;
  }
}

Status CLCommandQueue::Submit(const CLKernel& kernel, const cl_int dim,
                              const size_t* global_size,
                              const size_t* local_size, cl_event* event) {
  if (queue == nullptr) {
    return Status(ErrorCode::kInvalidState,
                  "CLCommandQueue requires successful initialization");
  }

  cl_int err = clEnqueueNDRangeKernel(queue, kernel.kernel, dim, nullptr, global_size, local_size,
                                      0, nullptr, event);
  if (err != CL_SUCCESS) {
    return Status(ErrorCode::kBackend, "clEnqueueNDRangeKernel failed",
                  "OpenCL", err);
  }
  return Status();
}

Status CLCommandQueue::Finish() {
  if (queue == nullptr) {
    return Status(ErrorCode::kInvalidState,
                  "CLCommandQueue requires successful initialization");
  }

  cl_int err = clFinish(queue);
  if (err != CL_SUCCESS) {
    return Status(ErrorCode::kBackend, "clFinish failed", "OpenCL", err);
  }
  return Status();
}

Status CLCommandQueue::ReadBuffer(const CLBuffer& buffer, void* dst,
                                  size_t bytes, size_t offset) {
  if (queue == nullptr) {
    return Status(ErrorCode::kInvalidState,
                  "CLCommandQueue requires successful initialization");
  }

  cl_int err =
      clEnqueueReadBuffer(queue, buffer.buffer, CL_TRUE, offset, bytes, dst, 0, nullptr, nullptr);
  if (err != CL_SUCCESS) {
    return Status(ErrorCode::kBackend, "clEnqueueReadBuffer failed", "OpenCL",
                  err);
  }
  return Status();
}

}  // namespace opencl
}  // namespace core
