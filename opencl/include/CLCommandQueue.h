#pragma once

#include "CLBuffer.h"
#include "CLContext.h"
#include "CLKernel.h"

namespace core {
namespace opencl {

class CLCommandQueue {
 public:
  CLCommandQueue(CLContext* context, const cl_command_queue_properties properties = 0);
  ~CLCommandQueue();

  // Enqueue a 1D NDRange kernel with optional local size (nullptr uses implementation default)
  void Submit(const CLKernel& kernel, const cl_int dim, const size_t* global_size,
              const size_t* local_size, cl_event* event);

  // Waits for all previously enqueued commands in the queue to finish
  void Finish();

  // Blocking read from a buffer into host memory
  void ReadBuffer(const CLBuffer& buffer, void* dst, size_t bytes, size_t offset = 0);

  cl_command_queue queue = nullptr;

 private:
  CLContext* context_ = nullptr;
};

}  // namespace opencl
}  // namespace core
