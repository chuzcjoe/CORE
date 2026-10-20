#pragma once

#include <memory>

#include "CLBuffer.h"
#include "CLContext.h"
#include "CLKernel.h"
#include "Status.h"

namespace core {
namespace opencl {

class CLCommandQueue {
 public:
  CLCommandQueue(const CLCommandQueue&) = delete;
  CLCommandQueue& operator=(const CLCommandQueue&) = delete;
  CLCommandQueue() = default;

  static Result<std::unique_ptr<CLCommandQueue>> Create(
      CLContext* context, const cl_command_queue_properties properties = 0);
  Status Init(CLContext* context,
              const cl_command_queue_properties properties = 0);
  ~CLCommandQueue();

  // Enqueue a 1D NDRange kernel with optional local size (nullptr uses implementation default)
  Status Submit(const CLKernel& kernel, const cl_int dim,
                const size_t* global_size = nullptr,
                const size_t* local_size = nullptr, cl_event* event = nullptr);

  // Waits for all previously enqueued commands in the queue to finish
  Status Finish();

  // Blocking read from a buffer into host memory
  Status ReadBuffer(const CLBuffer& buffer, void* dst, size_t bytes,
                    size_t offset = 0);

  cl_command_queue queue = nullptr;

 private:
  CLContext* context_ = nullptr;
  bool initialized_ = false;
};

}  // namespace opencl
}  // namespace core
