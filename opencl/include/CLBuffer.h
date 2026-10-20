#pragma once

#include <cstddef>
#include <memory>

#include "CLContext.h"
#include "Status.h"

namespace core {
namespace opencl {

class CLBuffer {
 public:
  CLBuffer(const CLBuffer&) = delete;
  CLBuffer& operator=(const CLBuffer&) = delete;
  CLBuffer() = default;
  // Create a buffer of given size and flags. Optionally initialize from host_ptr.

  static Result<std::unique_ptr<CLBuffer>> Create(
      CLContext* context, const size_t buffer_size,
      const cl_mem_flags flags = CL_MEM_READ_WRITE, void* host_ptr = nullptr);
  Status Init(CLContext* context, const size_t buffer_size,
              const cl_mem_flags flags = CL_MEM_READ_WRITE,
              void* host_ptr = nullptr);
  ~CLBuffer();

  cl_mem buffer = nullptr;
  size_t size = 0;

  // Map the buffer into host address space and return a typed pointer.
  // bytes == 0 maps the range [offset, size).
  template <typename T>
  Result<T*> MapBuffer(cl_command_queue command_queue,
                       cl_map_flags map_flags = CL_MAP_READ | CL_MAP_WRITE,
                       size_t offset = 0, size_t bytes = 0,
                       cl_bool blocking_map = CL_TRUE) const {
    if (command_queue == nullptr) {
      return Result<T*>(Status(ErrorCode::kInvalidArgument,
                               "MapBuffer requires a valid command queue"));
    }
    if (offset > size) {
      return Result<T*>(
          Status(ErrorCode::kOutOfRange, "MapBuffer offset is out of bounds"));
    }
    const size_t map_size = (bytes == 0) ? (size - offset) : bytes;
    if (map_size == 0 || map_size > size - offset) {
      return Result<T*>(
          Status(ErrorCode::kOutOfRange, "MapBuffer range is out of bounds"));
    }

    cl_int err = CL_SUCCESS;
    void* ptr = clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, map_size,
                                   0, nullptr, nullptr, &err);
    if (err != CL_SUCCESS || ptr == nullptr) {
      return Result<T*>(Status(ErrorCode::kBackend, "clEnqueueMapBuffer failed",
                               "OpenCL", err));
    }
    return Result<T*>(static_cast<T*>(ptr));
  }

  // Unmap a previously mapped pointer.
  Status UnmapBuffer(cl_command_queue command_queue, void* mapped_ptr) const {
    if (command_queue == nullptr || mapped_ptr == nullptr) {
      return Status(ErrorCode::kInvalidArgument,
                    "UnmapBuffer requires a valid queue and pointer");
    }
    cl_int err = clEnqueueUnmapMemObject(command_queue, buffer, mapped_ptr, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
      return Status(ErrorCode::kBackend, "clEnqueueUnmapMemObject failed",
                    "OpenCL", err);
    }
    return Status();
  }

 private:
  CLContext* context_ = nullptr;
  bool initialized_ = false;
};

}  // namespace opencl
}  // namespace core
