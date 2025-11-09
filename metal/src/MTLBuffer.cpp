#include "MTLBuffer.h"

namespace core {
namespace metal {

MTLBuffer::MTLBuffer(MTLContext* context, MTL::ResourceOptions options)
    : context_(context), metal_buffer_(nullptr), resource_options_(options) {}

MTLBuffer::~MTLBuffer() {
  if (metal_buffer_) {
    metal_buffer_->release();
  }
}

void MTLBuffer::CreateBuffer(const void* data, size_t data_size) {
  metal_buffer_ = context_->device()->newBuffer(data, data_size, resource_options_);
}

}  // namespace metal
}  // namespace core
