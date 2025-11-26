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
  CLBuffer(CLContext* context, const size_t size, const cl_mem_flags flags = CL_MEM_READ_WRITE,
           void* host_ptr = nullptr);
  ~CLBuffer();

  cl_mem buffer = nullptr;
  size_t size = 0;

  // Map the buffer into host address space and return a typed pointer.
  // bytes == 0 maps the range [offset, size).
  template <typename T>
  T* MapBuffer(cl_command_queue command_queue, cl_map_flags map_flags = CL_MAP_READ | CL_MAP_WRITE,
               size_t offset = 0, size_t bytes = 0, cl_bool blocking_map = CL_TRUE) const {
    if (command_queue == nullptr) {
      throw std::runtime_error("MapBuffer requires a valid command queue");
    }
    if (offset > size) {
      throw std::runtime_error("MapBuffer offset is out of bounds");
    }
    const size_t map_size = (bytes == 0) ? (size - offset) : bytes;
    if (map_size == 0 || offset + map_size > size) {
      throw std::runtime_error("MapBuffer range is out of bounds");
    }

    cl_int err = CL_SUCCESS;
    void* ptr = clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, map_size,
                                   0, nullptr, nullptr, &err);
    if (err != CL_SUCCESS || ptr == nullptr) {
      throw std::runtime_error("clEnqueueMapBuffer failed");
    }
    return static_cast<T*>(ptr);
  }

  // Unmap a previously mapped pointer.
  void UnmapBuffer(cl_command_queue command_queue, void* mapped_ptr) const {
    if (command_queue == nullptr || mapped_ptr == nullptr) {
      throw std::runtime_error("UnmapBuffer requires a valid queue and pointer");
    }
    cl_int err = clEnqueueUnmapMemObject(command_queue, buffer, mapped_ptr, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
      throw std::runtime_error("clEnqueueUnmapMemObject failed");
    }
  }

 private:
  CLContext* context_ = nullptr;
};

}  // namespace opencl
}  // namespace core
