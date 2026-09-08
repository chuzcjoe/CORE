#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include "RenderTriangle.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSwapChain.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

// This demo draws a triangle in a glfw window using vulkan graphic pipeline
const uint32_t kWidth = 800;
const uint32_t kHeight = 600;

int main() {
  VkSurfaceKHR window_surface = VK_NULL_HANDLE;
  GLFWwindow* window;
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    throw std::runtime_error("failed to create window");
  }

  core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Graphics;
  core::vulkan::VulkanContext context(true, queue_family_type, nullptr);
  std::unique_ptr<core::vulkan::VulkanSwapChain> swap_chain;
  if (glfwCreateWindowSurface(context.instance, window, nullptr, &window_surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface");
  } else {
    context.Init(window_surface);
  }

  if (window_surface != VK_NULL_HANDLE) {
    swap_chain = std::make_unique<core::vulkan::VulkanSwapChain>(&context, window_surface);
  }

  core::vulkan::VulkanCommandBuffer command_buffer(&context);
  core::vulkan::VulkanFence fence(&context);
  core::vulkan::VulkanSemaphore image_available_semaphore(&context);
  std::vector<std::unique_ptr<core::vulkan::VulkanSemaphore>> render_finished_semaphores;
  for (size_t i = 0; i < swap_chain->swapchain_images.size(); ++i) {
    render_finished_semaphores.push_back(std::make_unique<core::vulkan::VulkanSemaphore>(&context));
  }
  core::vulkan::VulkanFence in_flight_fence(&context);
  // Dynamic rendering
  core::vulkan::DynamicRenderingInfo dynamic_rendering_info{};
  dynamic_rendering_info.color_formats = {swap_chain->swapchain_image_format};
  std::unique_ptr<core::RenderTriangle> triangle =
      std::make_unique<core::RenderTriangle>(&context, dynamic_rendering_info);
  triangle->Init();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    // draw process
    vkWaitForFences(context.logical_device, 1, &(in_flight_fence.fence), VK_TRUE, UINT64_MAX);
    in_flight_fence.Reset();

    uint32_t image_index;
    vkAcquireNextImageKHR(context.logical_device, swap_chain->swapchain, UINT64_MAX,
                          image_available_semaphore.semaphore, VK_NULL_HANDLE, &image_index);

    triangle->UpdateUniformBuffer(swap_chain->swapchain_extent.width,
                                  swap_chain->swapchain_extent.height);

    // ========== Command buffer begin ==========
    command_buffer.Reset();

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(command_buffer.buffer(), &begin_info));
    swap_chain->TransitionImageLayout(command_buffer.buffer(), image_index,
                                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    triangle->DynamicRender(command_buffer.buffer(), swap_chain->swapchain_image_views[image_index],
                            swap_chain->swapchain_extent);

    swap_chain->TransitionImageLayout(command_buffer.buffer(), image_index,
                                      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore wait_semaphores[] = {image_available_semaphore.semaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    VkSemaphore signal_semaphores[] = {render_finished_semaphores[image_index]->semaphore};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    command_buffer.Submit(in_flight_fence.fence, submit_info);
    // ========== Command buffer end ==========

    // present
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    VkSwapchainKHR swapchains[] = {swap_chain->swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;

    vkQueuePresentKHR(context.present_queue(), &present_info);
  }
  vkDeviceWaitIdle(context.logical_device);

  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}
