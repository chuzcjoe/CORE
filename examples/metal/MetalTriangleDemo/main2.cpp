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
#include "MTLRenderPipeline.h"
#include "Metal/Metal.hpp"
#include "QuartzCore/QuartzCore.hpp"

constexpr unsigned int kWidth = 800;
constexpr unsigned int kHeight = 600;

// struct Vertex {
//   simd::float2 position;
//   simd::float3 color;
// };

// constexpr Vertex kVertices[] = {
//     {{-0.7f, -0.6f}, {1.0f, 0.2f, 0.2f}},
//     {{0.0f, 0.7f}, {0.2f, 0.9f, 0.3f}},
//     {{0.7f, -0.6f}, {0.2f, 0.4f, 1.0f}},
// };

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

  core::metal::MTLContext context;
  core::metal::MTLCommandQueue command_queue(&context);
  core::metal::MTLRenderPipeline pipeline(&context);

  context.LoadMetalShader("./examples/metal/MetalTriangleDemo/triangle.metal", "vertex_main",
                          "fragment_main");
  pipeline.CreateRenderPipeline(context.vertex_function(), context.fragment_function(),
                                MTL::PixelFormatBGRA8Unorm);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  pool->release();
  return 0;
}