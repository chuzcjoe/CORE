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
}

MTLContext::~MTLContext() {
  if (metal_layer_) {
    metal_layer_->release();
    metal_layer_ = nullptr;
  }
  if (metal_device_) {
    metal_device_->release();
    metal_device_ = nullptr;
  }
}

std::shared_ptr<MTL::Library> MTLContext::loadMetallib(MTL::Device* device, const char* path) {
  std::string p(path);
  NS::Error* error = nullptr;
  MTL::Library* library = nullptr;

  auto ends_with = [](const std::string& s, const char* suf) -> bool {
    const size_t n = std::strlen(suf);
    return s.size() >= n && s.compare(s.size() - n, n, suf) == 0;
  };

  if (ends_with(p, ".metal")) {
    // Compile from source at runtime
    std::ifstream file(path, std::ios::in | std::ios::binary);
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
    library = device->newLibrary(source, opts, &error);
    opts->release();
  } else {
    // Load a precompiled metallib from file path
    NS::String* nsPath = NS::String::string(path, NS::UTF8StringEncoding);
    library = device->newLibrary(nsPath, &error);
  }

  if (error || library == nullptr) {
    throw std::runtime_error("Failed to create Metal library (source or metallib)");
  }
  return std::shared_ptr<MTL::Library>(library, [](MTL::Library* p) { p->release(); });
}

}  // namespace metal
}  // namespace core
