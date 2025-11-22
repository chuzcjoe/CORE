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
#include "MTLBuffer.h"
#include "MTLCommandQueue.h"
#include "MTLContext.h"
#include "MTLRenderPipeline.h"
#include "MTLTexture.h"
#include "Metal/Metal.hpp"
#include "QuartzCore/QuartzCore.hpp"

constexpr unsigned int kWidth = 800;
constexpr unsigned int kHeight = 600;

struct VertexData {
  simd::float4 position;
  simd::float2 texture_coordinate;
};

constexpr VertexData square_vertices[]{
    {{-0.5, -0.5, 0.5, 1.0f}, {0.0f, 0.0f}}, {{-0.5, 0.5, 0.5, 1.0f}, {0.0f, 1.0f}},
    {{0.5, 0.5, 0.5, 1.0f}, {1.0f, 1.0f}},   {{-0.5, -0.5, 0.5, 1.0f}, {0.0f, 0.0f}},
    {{0.5, 0.5, 0.5, 1.0f}, {1.0f, 1.0f}},   {{0.5, -0.5, 0.5, 1.0f}, {1.0f, 0.0f}}};

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

  std::unique_ptr<core::metal::MTLContext> context =
      std::make_unique<core::metal::MTLContext>(window);
  std::unique_ptr<core::metal::MTLCommandQueue> command_queue =
      std::make_unique<core::metal::MTLCommandQueue>(context.get());
  std::unique_ptr<core::metal::MTLRenderPipeline> pipeline =
      std::make_unique<core::metal::MTLRenderPipeline>(context.get());
  std::unique_ptr<core::metal::MTLBuffer> buffer =
      std::make_unique<core::metal::MTLBuffer>(context.get(), MTL::ResourceStorageModeShared);
  std::unique_ptr<core::metal::MTLTexture> texture =
      std::make_unique<core::metal::MTLTexture>(context.get());

  texture->LoadTextureFromFile("./examples/data/mc_grass.jpeg");
  buffer->CreateBuffer(square_vertices, sizeof(square_vertices));
  pipeline->CreateRenderPipeline("./examples/data/square.metal", MTL::PixelFormatBGRA8Unorm);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // render
    NS::AutoreleasePool* frame_pool = NS::AutoreleasePool::alloc()->init();
    CA::MetalDrawable* drawable = context->layer()->nextDrawable();
    if (!drawable) {
      frame_pool->release();
      break;
    }

    pipeline->SetTexture(drawable);
    MTL::CommandBuffer* command_buffer = command_queue->command_buffer();
    MTL::RenderCommandEncoder* encoder =
        command_buffer->renderCommandEncoder(pipeline->render_pass_descriptor());

    encoder->setRenderPipelineState(pipeline->pipeline_state());
    encoder->setVertexBuffer(buffer->buffer(), 0, 0);
    encoder->setFragmentTexture(texture->texture(), 0);
    encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, static_cast<NS::UInteger>(0),
                            static_cast<NS::UInteger>(6));
    encoder->endEncoding();
    command_buffer->presentDrawable(drawable);
    command_buffer->commit();
    command_buffer->waitUntilCompleted();
    frame_pool->release();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  pool->release();
  return 0;
}