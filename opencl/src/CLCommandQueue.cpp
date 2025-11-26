#include "CLCommandQueue.h"

#include <stdexcept>

#include "CLLoader.h"

namespace core {
namespace opencl {

CLCommandQueue::CLCommandQueue(CLContext* context, const cl_command_queue_properties properties)
    : context_(context) {
  cl_int err = CL_SUCCESS;
  queue = clCreateCommandQueue(context_->context, context_->device, properties, &err);
  if (err != CL_SUCCESS || !queue) {
    throw std::runtime_error("clCreateCommandQueue failed");
  }
}

CLCommandQueue::~CLCommandQueue() {
  if (queue != nullptr) {
    clReleaseCommandQueue(queue);
    queue = nullptr;
  }
}

void CLCommandQueue::Submit(const CLKernel& kernel, const cl_int dim, const size_t* global_size,
                            const size_t* local_size, cl_event* event) {
  cl_int err = clEnqueueNDRangeKernel(queue, kernel.kernel, dim, nullptr, global_size, local_size,
                                      0, nullptr, event);
  if (err != CL_SUCCESS) {
    throw std::runtime_error("clEnqueueNDRangeKernel failed");
  }
}

void CLCommandQueue::Finish() {
  cl_int err = clFinish(queue);
  if (err != CL_SUCCESS) {
    throw std::runtime_error("clFinish failed");
  }
}

void CLCommandQueue::ReadBuffer(const CLBuffer& buffer, void* dst, size_t bytes, size_t offset) {
  cl_int err =
      clEnqueueReadBuffer(queue, buffer.buffer, CL_TRUE, offset, bytes, dst, 0, nullptr, nullptr);
  if (err != CL_SUCCESS) {
    throw std::runtime_error("clEnqueueReadBuffer failed");
  }
}

}  // namespace opencl
}  // namespace core
