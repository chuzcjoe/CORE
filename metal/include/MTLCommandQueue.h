#pragma once

#include "MTLContext.h"

namespace core {
namespace metal {

class MTLCommandQueue {
 public:
  explicit MTLCommandQueue(MTLContext* context);
  ~MTLCommandQueue();

  MTL::CommandBuffer* command_buffer() { return command_queue_->commandBuffer(); }

 private:
  MTLContext* context_;
  MTL::CommandQueue* command_queue_;
};

}  // namespace metal
}  // namespace core
