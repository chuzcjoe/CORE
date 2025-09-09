#include <GLFW/glfw3.h>

#include "GraphicTriangle.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSync.h"

// This demo draws a triangle in a glfw window using vulkan graphic pipeline
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

int main() {
  // core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Graphics;
  // core::vulkan::VulkanContext context(true, queue_family_type);
  // core::vulkan::VulkanCommandBuffer command_buffer(&context);
  // core::vulkan::VulkanFence fence(&context);
  // VkFormat format = VK_FORMAT_R32G32B32_SFLOAT;
  // core::vulkan::VulkanRenderPass render_pass(&context, format);

  GLFWwindow* window;
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

  if (!window) {
    glfwTerminate();
    return EXIT_FAILURE;
  }

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  // init Vulkan
  // const auto graphic_triangle = std::make_unique<core::GraphicTriangle>(&context, render_pass);

  return EXIT_SUCCESS;
}