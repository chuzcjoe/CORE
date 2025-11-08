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

  void LoadMetalShader(const std::string shader_path, const std::string vertex_fn_name,
                       const std::string fragment_fn_name);

  MTL::Device* device() const { return metal_device_; }
  CA::MetalLayer* layer() const { return metal_layer_; }
  MTL::Function* vertex_function() const { return vertex_fn_; }
  MTL::Function* fragment_function() const { return fragment_fn_; }

 private:
  MTL::Device* metal_device_;
  CA::MetalLayer* metal_layer_;

  MTL::Function* vertex_fn_;
  MTL::Function* fragment_fn_;
};

}  // namespace metal
}  // namespace core