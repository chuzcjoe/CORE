#include "MTLCommandQueue.h"

namespace core {
namespace metal {

MTLCommandQueue::MTLCommandQueue(MTLContext* context)
    : context_(context),
      command_queue_(context_->device()->newCommandQueue()),
      command_buffer_(command_queue_->commandBuffer()) {}

MTLCommandQueue::~MTLCommandQueue() {
  if (command_buffer_) {
    command_buffer_->release();
  }
  if (command_queue_) {
    command_queue_->release();
  }
}

}  // namespace metal
}  // namespace core