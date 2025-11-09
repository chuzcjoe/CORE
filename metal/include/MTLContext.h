#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include <objc/message.h>
#include <objc/objc.h>

#include <fstream>

#include "Foundation/Foundation.hpp"
#include "Metal/Metal.hpp"
#include "QuartzCore/QuartzCore.hpp"

namespace core {
namespace metal {

template <typename Ret, typename... Args>
Ret ObjcCall(id obj, const char* selector, Args... args) {
  SEL sel = sel_registerName(selector);
  auto fn = reinterpret_cast<Ret (*)(id, SEL, Args...)>(objc_msgSend);
  return fn(obj, sel, args...);
}

class MTLContext {
 public:
  MTLContext();
  MTLContext(GLFWwindow* window);
  ~MTLContext();

  MTL::Device* device() const { return metal_device_; }
  CA::MetalLayer* layer() const { return metal_layer_; }
  MTL::Function* vertex_function() const { return vertex_fn_; }
  MTL::Function* fragment_function() const { return fragment_fn_; }

 private:
  void CreateMetalLayerForWindow(GLFWwindow* window);

  MTL::Device* metal_device_;
  CA::MetalLayer* metal_layer_;

  MTL::Function* vertex_fn_;
  MTL::Function* fragment_fn_;
};

}  // namespace metal
}  // namespace core