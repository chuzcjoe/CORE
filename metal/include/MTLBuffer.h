#pragma once

#include <simd/simd.h>

#include "MTLContext.h"

namespace core {
namespace metal {

class MTLBuffer {
 public:
  MTLBuffer(MTLContext* context, MTL::ResourceOptions options = MTL::ResourceStorageModeShared);
  ~MTLBuffer();

  void CreateBuffer(simd::float3* data, size_t data_size);

  MTL::Buffer* buffer() const { return metal_buffer_; }

 private:
  MTLContext* context_;
  MTL::Buffer* metal_buffer_;
  MTL::ResourceOptions resource_options_;
};

}  // namespace metal
}  // namespace core
