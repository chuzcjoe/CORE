// #define GLFW_INCLUDE_NONE
// #include <GLFW/glfw3.h>
// #define GLFW_EXPOSE_NATIVE_COCOA
// #include <GLFW/glfw3native.h>

// #include <simd/simd.h>

// #include <iostream>
// #include <stdexcept>
// #include <string>

// #include "Foundation/Foundation.hpp"
// #include "Metal/Metal.hpp"
// #include "QuartzCore/QuartzCore.hpp"
// #include <objc/objc.h>
// #include <objc/message.h>

// namespace {

// constexpr unsigned int kWidth = 800;
// constexpr unsigned int kHeight = 600;

// struct Vertex {
//   simd::float2 position;
//   simd::float3 color;
// };

// constexpr Vertex kVertices[] = {
//     {{-0.7f, -0.6f}, {1.0f, 0.2f, 0.2f}},
//     {{0.0f, 0.7f}, {0.2f, 0.9f, 0.3f}},
//     {{0.7f, -0.6f}, {0.2f, 0.4f, 1.0f}},
// };

// constexpr const char* kShaderSource = R"METAL(
// using namespace metal;

// struct VertexIn {
//   float2 position;
//   float3 color;
// };

// struct VertexOut {
//   float4 position [[position]];
//   float3 color;
// };

// vertex VertexOut vertex_main(uint vertexId [[vertex_id]],
//                              constant VertexIn* vertices [[buffer(0)]]) {
//   VertexOut out;
//   VertexIn v = vertices[vertexId];
//   out.position = float4(v.position, 0.0, 1.0);
//   out.color = v.color;
//   return out;
// }

// fragment float4 fragment_main(VertexOut in [[stage_in]]) {
//   return float4(in.color, 1.0);
// }
// )METAL";

// struct MetalResources {
//   MTL::Device* device = nullptr;
//   MTL::CommandQueue* command_queue = nullptr;
//   MTL::RenderPipelineState* pipeline_state = nullptr;
//   MTL::Buffer* vertex_buffer = nullptr;
//   CA::MetalLayer* metal_layer = nullptr;
// };

// template <typename Ret, typename... Args>
// Ret ObjcCall(id obj, const char* selector, Args... args) {
//   SEL sel = sel_registerName(selector);
//   auto fn = reinterpret_cast<Ret (*)(id, SEL, Args...)>(objc_msgSend);
//   return fn(obj, sel, args...);
// }

// CA::MetalLayer* CreateLayerForWindow(GLFWwindow* window) {
//   id ns_window = glfwGetCocoaWindow(window);
//   if (!ns_window) return nullptr;
//   id content_view = ObjcCall<id>(ns_window, "contentView");
//   ObjcCall<void>(content_view, "setWantsLayer:", YES);
//   CA::MetalLayer* layer = CA::MetalLayer::layer()->retain();
//   ObjcCall<void>(content_view, "setLayer:", reinterpret_cast<id>(layer));
//   return layer;
// }

// void UpdateDrawableSize(GLFWwindow* window, CA::MetalLayer* layer) {
//   if (!layer) return;
//   int width = 0;
//   int height = 0;
//   glfwGetFramebufferSize(window, &width, &height);
//   float scale_x = 1.0f;
//   float scale_y = 1.0f;
//   glfwGetWindowContentScale(window, &scale_x, &scale_y);
//   layer->setDrawableSize(CGSizeMake(width * scale_x, height * scale_y));
// }

// void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
//   auto* resources = static_cast<MetalResources*>(glfwGetWindowUserPointer(window));
//   if (!resources || !resources->metal_layer) return;
//   float scale_x = 1.0f;
//   float scale_y = 1.0f;
//   glfwGetWindowContentScale(window, &scale_x, &scale_y);
//   resources->metal_layer->setDrawableSize(CGSizeMake(width * scale_x, height * scale_y));
// }

// MTL::Library* CompileLibraryFromSource(MTL::Device* device, const char* source) {
//   NS::Error* error = nullptr;
//   NS::String* ns_source = NS::String::string(source, NS::UTF8StringEncoding);
//   MTL::CompileOptions* options = MTL::CompileOptions::alloc()->init();
//   MTL::Library* library = device->newLibrary(ns_source, options, &error);
//   options->release();
//   if (error || library == nullptr) {
//     std::string message = "Failed to compile Metal shader";
//     if (error && error->localizedDescription()) {
//       message += ": ";
//       message += error->localizedDescription()->utf8String();
//     }
//     throw std::runtime_error(message);
//   }
//   return library;
// }

// void RenderFrame(MetalResources& resources) {
//   if (!resources.metal_layer) return;
//   NS::AutoreleasePool* frame_pool = NS::AutoreleasePool::alloc()->init();

//   CA::MetalDrawable* drawable = resources.metal_layer->nextDrawable();
//   if (!drawable) {
//     frame_pool->release();
//     return;
//   }

//   MTL::RenderPassDescriptor* pass_descriptor = MTL::RenderPassDescriptor::renderPassDescriptor();
//   auto* color_attachment = pass_descriptor->colorAttachments()->object(0);
//   color_attachment->setTexture(drawable->texture());
//   color_attachment->setLoadAction(MTL::LoadActionClear);
//   color_attachment->setStoreAction(MTL::StoreActionStore);
//   color_attachment->setClearColor(MTL::ClearColor(0.05, 0.05, 0.1, 1.0));

//   MTL::CommandBuffer* command_buffer = resources.command_queue->commandBuffer();
//   MTL::RenderCommandEncoder* encoder = command_buffer->renderCommandEncoder(pass_descriptor);
//   encoder->setRenderPipelineState(resources.pipeline_state);
//   encoder->setVertexBuffer(resources.vertex_buffer, 0, 0);
//   encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, static_cast<NS::UInteger>(0),
//                           static_cast<NS::UInteger>(3));
//   encoder->endEncoding();

//   command_buffer->presentDrawable(drawable);
//   command_buffer->commit();

//   frame_pool->release();
// }

// }  // namespace

// int main() {
//   NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

//   if (!glfwInit()) {
//     pool->release();
//     throw std::runtime_error("Failed to initialize GLFW");
//   }

//   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//   glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

//   GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "Metal Triangle", nullptr, nullptr);
//   if (!window) {
//     glfwTerminate();
//     pool->release();
//     throw std::runtime_error("Failed to create GLFW window");
//   }

//   MetalResources resources;

//   resources.device = MTL::CreateSystemDefaultDevice();
//   if (!resources.device) {
//     throw std::runtime_error("Metal is not supported on this system");
//   }

//   resources.command_queue = resources.device->newCommandQueue();
//   if (!resources.command_queue) {
//     throw std::runtime_error("Failed to create Metal command queue");
//   }

//   MTL::Library* library = CompileLibraryFromSource(resources.device, kShaderSource);
//   MTL::Function* vertex_fn = library->newFunction(MTLSTR("vertex_main"));
//   MTL::Function* fragment_fn = library->newFunction(MTLSTR("fragment_main"));

//   MTL::RenderPipelineDescriptor* pipeline_desc = MTL::RenderPipelineDescriptor::alloc()->init();
//   pipeline_desc->setVertexFunction(vertex_fn);
//   pipeline_desc->setFragmentFunction(fragment_fn);
//   pipeline_desc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

//   NS::Error* pipeline_error = nullptr;
//   resources.pipeline_state = resources.device->newRenderPipelineState(pipeline_desc,
//   &pipeline_error); pipeline_desc->release(); vertex_fn->release(); fragment_fn->release();
//   library->release();

//   if (pipeline_error || !resources.pipeline_state) {
//     std::string message = "Failed to create pipeline state";
//     if (pipeline_error && pipeline_error->localizedDescription()) {
//       message += ": ";
//       message += pipeline_error->localizedDescription()->utf8String();
//     }
//     throw std::runtime_error(message);
//   }

//   resources.vertex_buffer = resources.device->newBuffer(kVertices, sizeof(kVertices),
//                                                         MTL::ResourceStorageModeShared);
//   if (!resources.vertex_buffer) {
//     throw std::runtime_error("Failed to create vertex buffer");
//   }

//   resources.metal_layer = CreateLayerForWindow(window);
//   if (!resources.metal_layer) {
//     throw std::runtime_error("Failed to acquire CAMetalLayer from GLFW window");
//   }
//   resources.metal_layer->retain();
//   resources.metal_layer->setDevice(resources.device);
//   resources.metal_layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
//   resources.metal_layer->setFramebufferOnly(true);
//   UpdateDrawableSize(window, resources.metal_layer);

//   glfwSetWindowUserPointer(window, &resources);
//   glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

//   while (!glfwWindowShouldClose(window)) {
//     glfwPollEvents();
//     RenderFrame(resources);
//   }

//   if (resources.metal_layer) resources.metal_layer->release();
//   if (resources.vertex_buffer) resources.vertex_buffer->release();
//   if (resources.pipeline_state) resources.pipeline_state->release();
//   if (resources.command_queue) resources.command_queue->release();
//   if (resources.device) resources.device->release();

//   glfwDestroyWindow(window);
//   glfwTerminate();

//   pool->release();
//   return 0;
// }

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include <objc/message.h>
#include <objc/objc.h>
#include <simd/simd.h>

#include <iostream>
#include <stdexcept>
#include <string>

#include "Foundation/Foundation.hpp"
#include "MTLCommandQueue.h"
#include "MTLContext.h"
#include "Metal/Metal.hpp"
#include "QuartzCore/QuartzCore.hpp"

namespace {

constexpr unsigned int kWidth = 800;
constexpr unsigned int kHeight = 600;

struct Vertex {
  simd::float2 position;
  simd::float3 color;
};

constexpr Vertex kVertices[] = {
    {{-0.7f, -0.6f}, {1.0f, 0.2f, 0.2f}},
    {{0.0f, 0.7f}, {0.2f, 0.9f, 0.3f}},
    {{0.7f, -0.6f}, {0.2f, 0.4f, 1.0f}},
};

constexpr const char* kShaderSource = R"METAL(
using namespace metal;

struct VertexIn {
  float2 position;
  float3 color;
};

struct VertexOut {
  float4 position [[position]];
  float3 color;
};

vertex VertexOut vertex_main(uint vertexId [[vertex_id]],
                             constant VertexIn* vertices [[buffer(0)]]) {
  VertexOut out;
  VertexIn v = vertices[vertexId];
  out.position = float4(v.position, 0.0, 1.0);
  out.color = v.color;
  return out;
}

fragment float4 fragment_main(VertexOut in [[stage_in]]) {
  return float4(in.color, 1.0);
}
)METAL";

struct MetalResources {
  MTL::Device* device = nullptr;
  MTL::CommandQueue* command_queue = nullptr;
  MTL::RenderPipelineState* pipeline_state = nullptr;
  MTL::Buffer* vertex_buffer = nullptr;
  CA::MetalLayer* metal_layer = nullptr;
};

template <typename Ret, typename... Args>
Ret ObjcCall(id obj, const char* selector, Args... args) {
  SEL sel = sel_registerName(selector);
  auto fn = reinterpret_cast<Ret (*)(id, SEL, Args...)>(objc_msgSend);
  return fn(obj, sel, args...);
}

CA::MetalLayer* CreateLayerForWindow(GLFWwindow* window) {
  id ns_window = glfwGetCocoaWindow(window);
  if (!ns_window) return nullptr;
  id content_view = ObjcCall<id>(ns_window, "contentView");
  ObjcCall<void>(content_view, "setWantsLayer:", YES);
  CA::MetalLayer* layer = CA::MetalLayer::layer()->retain();
  ObjcCall<void>(content_view, "setLayer:", reinterpret_cast<id>(layer));
  return layer;
}

void UpdateDrawableSize(GLFWwindow* window, CA::MetalLayer* layer) {
  if (!layer) return;
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  float scale_x = 1.0f;
  float scale_y = 1.0f;
  glfwGetWindowContentScale(window, &scale_x, &scale_y);
  layer->setDrawableSize(CGSizeMake(width * scale_x, height * scale_y));
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
  auto* resources = static_cast<MetalResources*>(glfwGetWindowUserPointer(window));
  if (!resources || !resources->metal_layer) return;
  float scale_x = 1.0f;
  float scale_y = 1.0f;
  glfwGetWindowContentScale(window, &scale_x, &scale_y);
  resources->metal_layer->setDrawableSize(CGSizeMake(width * scale_x, height * scale_y));
}

MTL::Library* CompileLibraryFromSource(MTL::Device* device, const char* source) {
  NS::Error* error = nullptr;
  NS::String* ns_source = NS::String::string(source, NS::UTF8StringEncoding);
  MTL::CompileOptions* options = MTL::CompileOptions::alloc()->init();
  MTL::Library* library = device->newLibrary(ns_source, options, &error);
  options->release();
  if (error || library == nullptr) {
    std::string message = "Failed to compile Metal shader";
    if (error && error->localizedDescription()) {
      message += ": ";
      message += error->localizedDescription()->utf8String();
    }
    throw std::runtime_error(message);
  }
  return library;
}

void RenderFrame(MetalResources& resources) {
  if (!resources.metal_layer) return;
  NS::AutoreleasePool* frame_pool = NS::AutoreleasePool::alloc()->init();

  CA::MetalDrawable* drawable = resources.metal_layer->nextDrawable();
  if (!drawable) {
    frame_pool->release();
    return;
  }

  MTL::RenderPassDescriptor* pass_descriptor = MTL::RenderPassDescriptor::renderPassDescriptor();
  auto* color_attachment = pass_descriptor->colorAttachments()->object(0);
  color_attachment->setTexture(drawable->texture());
  color_attachment->setLoadAction(MTL::LoadActionClear);
  color_attachment->setStoreAction(MTL::StoreActionStore);
  color_attachment->setClearColor(MTL::ClearColor(0.05, 0.05, 0.1, 1.0));

  MTL::CommandBuffer* command_buffer = resources.command_queue->commandBuffer();
  MTL::RenderCommandEncoder* encoder = command_buffer->renderCommandEncoder(pass_descriptor);
  encoder->setRenderPipelineState(resources.pipeline_state);
  encoder->setVertexBuffer(resources.vertex_buffer, 0, 0);
  encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, static_cast<NS::UInteger>(0),
                          static_cast<NS::UInteger>(3));
  encoder->endEncoding();

  command_buffer->presentDrawable(drawable);
  command_buffer->commit();

  frame_pool->release();
}

}  // namespace

int main() {
  NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

  if (!glfwInit()) {
    pool->release();
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "Metal Triangle", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    pool->release();
    throw std::runtime_error("Failed to create GLFW window");
  }

  MetalResources resources;
  core::metal::MTLContext context;
  core::metal::MTLCommandQueue command_queue(&context);

  resources.device = MTL::CreateSystemDefaultDevice();
  resources.command_queue = resources.device->newCommandQueue();

  MTL::Library* library = CompileLibraryFromSource(resources.device, kShaderSource);
  MTL::Function* vertex_fn = library->newFunction(MTLSTR("vertex_main"));
  MTL::Function* fragment_fn = library->newFunction(MTLSTR("fragment_main"));

  context.LoadMetalShader("./examples/metal/MetalTriangleDemo/triangle.metal", "vertex_main",
                          "fragment_main");

  MTL::RenderPipelineDescriptor* pipeline_desc = MTL::RenderPipelineDescriptor::alloc()->init();
  pipeline_desc->setVertexFunction(vertex_fn);
  pipeline_desc->setFragmentFunction(fragment_fn);
  pipeline_desc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

  NS::Error* pipeline_error = nullptr;
  resources.pipeline_state =
      resources.device->newRenderPipelineState(pipeline_desc, &pipeline_error);
  pipeline_desc->release();
  vertex_fn->release();
  fragment_fn->release();
  library->release();
  lib->release();

  if (pipeline_error || !resources.pipeline_state) {
    std::string message = "Failed to create pipeline state";
    if (pipeline_error && pipeline_error->localizedDescription()) {
      message += ": ";
      message += pipeline_error->localizedDescription()->utf8String();
    }
    throw std::runtime_error(message);
  }

  resources.vertex_buffer =
      resources.device->newBuffer(kVertices, sizeof(kVertices), MTL::ResourceStorageModeShared);
  if (!resources.vertex_buffer) {
    throw std::runtime_error("Failed to create vertex buffer");
  }

  resources.metal_layer = CreateLayerForWindow(window);
  if (!resources.metal_layer) {
    throw std::runtime_error("Failed to acquire CAMetalLayer from GLFW window");
  }
  resources.metal_layer->retain();
  resources.metal_layer->setDevice(resources.device);
  resources.metal_layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
  resources.metal_layer->setFramebufferOnly(true);
  UpdateDrawableSize(window, resources.metal_layer);

  glfwSetWindowUserPointer(window, &resources);
  glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    RenderFrame(resources);
  }

  if (resources.metal_layer) resources.metal_layer->release();
  if (resources.vertex_buffer) resources.vertex_buffer->release();
  if (resources.pipeline_state) resources.pipeline_state->release();
  if (resources.command_queue) resources.command_queue->release();
  if (resources.device) resources.device->release();

  glfwDestroyWindow(window);
  glfwTerminate();

  pool->release();
  return 0;
}