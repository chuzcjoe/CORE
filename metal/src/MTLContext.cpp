#include "Status.h"

// Provide metal-cpp private implementation symbols in this TU
// so that callers linking against core_metal can resolve symbols
// like MTL::CreateSystemDefaultDevice and CA/NS selectors.
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include "MTLContext.h"

namespace core {
namespace metal {

Result<std::unique_ptr<MTLContext>> MTLContext::Create() {
  std::unique_ptr<MTLContext> object(new MTLContext());
  const Status status = object->Init();
  if (!status.ok()) {
    return Result<std::unique_ptr<MTLContext>>(status);
  }
  return Result<std::unique_ptr<MTLContext>>(std::move(object));
}

Status MTLContext::Init() {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "MTLContext initialization is single-use");
  }
  initialized_ = true;
  metal_device_ = nullptr;
  metal_layer_ = nullptr;

  metal_device_ = MTL::CreateSystemDefaultDevice();
  if (metal_device_ == nullptr) {
    return Status(ErrorCode::kUnsupported, "No Metal device available");
  }
  metal_layer_ = CA::MetalLayer::layer()->retain();
  metal_layer_->setDevice(metal_device_);
  metal_layer_->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
  metal_layer_->setFramebufferOnly(true);
  // TODO: fix hardcode size
  metal_layer_->setDrawableSize(CGSizeMake(800, 600));

  return Status();
}

Result<std::unique_ptr<MTLContext>> MTLContext::Create(GLFWwindow* window) {
  std::unique_ptr<MTLContext> object(new MTLContext());
  const Status status = object->Init(window);
  if (!status.ok()) {
    return Result<std::unique_ptr<MTLContext>>(status);
  }
  return Result<std::unique_ptr<MTLContext>>(std::move(object));
}

Status MTLContext::Init(GLFWwindow* window) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "MTLContext initialization is single-use");
  }
  initialized_ = true;
  metal_device_ = nullptr;
  metal_layer_ = nullptr;

  metal_device_ = MTL::CreateSystemDefaultDevice();
  if (metal_device_ == nullptr) {
    return Status(ErrorCode::kUnsupported, "No Metal device available");
  }
  {
    const core::Status status = CreateMetalLayerForWindow(window);
    if (!status.ok()) {
      return status;
    }
  }
  metal_layer_->setDevice(metal_device_);
  metal_layer_->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
  metal_layer_->setFramebufferOnly(true);
  metal_layer_->setDrawableSize(CGSizeMake(800, 600));

  return Status();
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

Status MTLContext::CreateMetalLayerForWindow(GLFWwindow* window) {
  if (window == nullptr) {
    return Status(ErrorCode::kInvalidArgument,
                  "A live GLFW window is required");
  }
  id ns_window = glfwGetCocoaWindow(window);
  if (!ns_window) {
    return Status(ErrorCode::kBackend,
                  "Failed to get Cocoa window from GLFWwindow");
  }
  id content_view = ObjcCall<id>(ns_window, "contentView");
  ObjcCall<void>(content_view, "setWantsLayer:", YES);
  metal_layer_ = CA::MetalLayer::layer()->retain();
  ObjcCall<void>(content_view, "setLayer:", reinterpret_cast<id>(metal_layer_));

  return Status();
}

}  // namespace metal
}  // namespace core
