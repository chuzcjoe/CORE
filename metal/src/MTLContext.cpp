// Provide metal-cpp private implementation symbols in this TU
// so that callers linking against core_metal can resolve symbols
// like MTL::CreateSystemDefaultDevice and CA/NS selectors.
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include "MTLContext.h"

namespace core {
namespace metal {

MTLContext::MTLContext() : metal_device_(nullptr), metal_layer_(nullptr) {
  metal_device_ = MTL::CreateSystemDefaultDevice();
  metal_layer_ = CA::MetalLayer::layer()->retain();
  metal_layer_->setDevice(metal_device_);
  metal_layer_->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
  metal_layer_->setFramebufferOnly(true);
  // TODO: fix hardcode size
  metal_layer_->setDrawableSize(CGSizeMake(800, 600));
}

MTLContext::MTLContext(GLFWwindow* window) : metal_device_(nullptr), metal_layer_(nullptr) {
  metal_device_ = MTL::CreateSystemDefaultDevice();
  CreateMetalLayerForWindow(window);
  metal_layer_->retain();
  metal_layer_->setDevice(metal_device_);
  metal_layer_->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
  metal_layer_->setFramebufferOnly(true);
  metal_layer_->setDrawableSize(CGSizeMake(800, 600));
}

MTLContext::~MTLContext() {
  if (vertex_fn_) {
    vertex_fn_->release();
    vertex_fn_ = nullptr;
  }
  if (fragment_fn_) {
    fragment_fn_->release();
    fragment_fn_ = nullptr;
  }
  if (metal_layer_) {
    metal_layer_->release();
    metal_layer_ = nullptr;
  }
  if (metal_device_) {
    metal_device_->release();
    metal_device_ = nullptr;
  }
}

void MTLContext::CreateMetalLayerForWindow(GLFWwindow* window) {
  id ns_window = glfwGetCocoaWindow(window);
  if (!ns_window) {
    throw std::runtime_error("Failed to get Cocoa window from GLFWwindow");
  }
  id content_view = ObjcCall<id>(ns_window, "contentView");
  ObjcCall<void>(content_view, "setWantsLayer:", YES);
  metal_layer_ = CA::MetalLayer::layer()->retain();
  ObjcCall<void>(content_view, "setLayer:", reinterpret_cast<id>(metal_layer_));
}

}  // namespace metal
}  // namespace core
