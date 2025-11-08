#pragma once

#include "MTLContext.h"

namespace core {
namespace metal {

class MTLCommandQueue {
 public:
  explicit MTLCommandQueue(MTLContext* context);
  ~MTLCommandQueue();

  MTL::CommandBuffer* command_buffer() { return command_buffer_; }

 private:
  MTLContext* context_;
  MTL::CommandQueue* command_queue_;
  // May need more command buffers in future
  MTL::CommandBuffer* command_buffer_;
};

}  // namespace metal
}  // namespace core
