#include <memory>

#include "Status.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include <objc/message.h>
#include <objc/objc.h>
#include <simd/simd.h>

#include <iostream>
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

core::Status RunApplication() {
  NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
  const auto release_pool = [](NS::AutoreleasePool* value) {
    value->release();
  };
  const std::unique_ptr<NS::AutoreleasePool, decltype(release_pool)> pool_owner(
      pool, release_pool);

  if (!glfwInit()) {
    return core::Status(core::ErrorCode::kBackend, "Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "Metal Triangle", nullptr, nullptr);
  if (!window) {
    glfwTerminate();

    return core::Status(core::ErrorCode::kBackend,
                        "Failed to create GLFW window");
  }
  const auto close_window = [](GLFWwindow* value) {
    glfwDestroyWindow(value);
    glfwTerminate();
  };
  const std::unique_ptr<GLFWwindow, decltype(close_window)> window_owner(
      window, close_window);

  auto created_resource_0 = core::metal::MTLContext::Create(window);
  if (!created_resource_0.ok()) {
    return created_resource_0.status();
  }

  std::unique_ptr<core::metal::MTLContext> context =
      std::move(*created_resource_0);
  std::unique_ptr<core::metal::MTLCommandQueue> command_queue =
      std::make_unique<core::metal::MTLCommandQueue>(context.get());
  std::unique_ptr<core::metal::MTLRenderPipeline> pipeline =
      std::make_unique<core::metal::MTLRenderPipeline>(context.get());
  std::unique_ptr<core::metal::MTLBuffer> buffer =
      std::make_unique<core::metal::MTLBuffer>(context.get(), MTL::ResourceStorageModeShared);
  std::unique_ptr<core::metal::MTLTexture> texture =
      std::make_unique<core::metal::MTLTexture>(context.get());

  {
    const core::Status status =
        texture->LoadTextureFromFile("./examples/data/mc_grass.jpeg");
    if (!status.ok()) {
      return status;
    }
  }
  buffer->CreateBuffer(square_vertices, sizeof(square_vertices));
  {
    const core::Status status = pipeline->CreateRenderPipeline(
        "./examples/data/square.metal", MTL::PixelFormatBGRA8Unorm);
    if (!status.ok()) {
      return status;
    }
  }

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

  return core::Status();
}
int main() {
  const core::Status status = RunApplication();
  if (!status.ok()) {
    std::fprintf(stderr, "%s\n", status.message().c_str());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
