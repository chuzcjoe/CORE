// clang-format off
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
// clang-format on

#include <simd/simd.h>

#include <iostream>

#include "MTLBuffer.h"
#include "MTLContext.h"

// settings
const unsigned int kWidth = 800;
const unsigned int kHeight = 600;

// clang-format off

int main() {
  core::metal::MTLContext context;
  core::metal::MTLBuffer buffer(&context);
  const auto metal_lib = core::metal::MTLContext::loadMetallib(context.device(), "examples/metal/MetalTriangleDemo/triangle.metal");

  printf("metal device: %s\n", context.device()->name()->utf8String());

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "Metal Window", NULL, NULL);
  if (window == nullptr) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  // vertex data
  simd::float3 vertices[] = {
        {-0.5f, -0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
        { 0.0f,  0.5f, 0.0f}
  };

  buffer.CreateBuffer(vertices, sizeof(vertices));
  
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwTerminate();

  return 0;
}