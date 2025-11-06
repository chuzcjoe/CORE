#pragma once

#include <fstream>

#include "Foundation/Foundation.hpp"
#include "Metal/Metal.hpp"
#include "QuartzCore/QuartzCore.hpp"

namespace core {
namespace metal {

class MTLContext {
 public:
  MTLContext();
  ~MTLContext();

  static std::shared_ptr<MTL::Library> loadMetallib(MTL::Device* device, const char* path);

  MTL::Device* device() const { return metal_device_; }
  CA::MetalLayer* layer() const { return metal_layer_; }

 private:
  MTL::Device* metal_device_;
  CA::MetalLayer* metal_layer_;
};

}  // namespace metal
}  // namespace core