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

void MTLContext::LoadMetalShader(const std::string shader_path, const std::string vertex_fn_name,
                                 const std::string fragment_fn_name) {
  NS::Error* error = nullptr;
  MTL::Library* library = nullptr;

  auto ends_with = [](const std::string& s, const char* suf) -> bool {
    const size_t n = std::strlen(suf);
    return s.size() >= n && s.compare(s.size() - n, n, suf) == 0;
  };

  if (ends_with(shader_path, ".metal")) {
    // Compile from source at runtime
    std::ifstream file(shader_path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("Failed to open .metal source file");
    }
    std::string src;
    file.seekg(0, std::ios::end);
    src.resize(static_cast<size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);
    file.read(src.data(), static_cast<std::streamsize>(src.size()));
    file.close();

    NS::String* source = NS::String::string(src.c_str(), NS::UTF8StringEncoding);
    MTL::CompileOptions* opts = MTL::CompileOptions::alloc()->init();
    library = metal_device_->newLibrary(source, opts, &error);
    opts->release();
  } else {
    // Load a precompiled metallib from file path
    NS::String* nsPath = NS::String::string(shader_path.c_str(), NS::UTF8StringEncoding);
    library = metal_device_->newLibrary(nsPath, &error);
  }

  if (error || library == nullptr) {
    throw std::runtime_error("Failed to create Metal library (source or metallib)");
  }
  NS::String* vname = NS::String::string(vertex_fn_name.c_str(), NS::UTF8StringEncoding);
  NS::String* fname = NS::String::string(fragment_fn_name.c_str(), NS::UTF8StringEncoding);
  vertex_fn_ = library->newFunction(vname);
  fragment_fn_ = library->newFunction(fname);

  if (vertex_fn_ == nullptr || fragment_fn_ == nullptr) {
    throw std::runtime_error("Failed to create Metal shader functions");
  }

  library->release();
}

}  // namespace metal
}  // namespace core
