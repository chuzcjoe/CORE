#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <chrono>
#include <cstdlib>
#include <memory>

#include "Physics.h"
#include "RenderCube.h"
#include "RenderFloor.h"
#include "VulkanCommandBuffer.h"
#include "VulkanImage.h"
#include "VulkanSwapChain.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

// Demo: a cube spawns mid-air, falls under gravity onto a checkerboard floor,
// bounces with restitution and friction, tumbles, and auto-resets once it
// settles. Dynamic rendering with a manually-managed depth attachment.
const uint32_t kWidth = 1280;
const uint32_t kHeight = 800;
const VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;

int main() {
  VkSurfaceKHR window_surface = VK_NULL_HANDLE;
  GLFWwindow* window;
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(kWidth, kHeight, "Physics Demo", nullptr, nullptr);
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

  // Manually-managed depth image — the swapchain's internal depth buffer is
  // only wired up for the traditional render-pass path, so for dynamic
  // rendering we own the depth attachment here.
  core::vulkan::VulkanImage depth_image(
      &context, swap_chain->swapchain_extent.width, swap_chain->swapchain_extent.height,
      kDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL);
  depth_image.TransitionDepthImageLayout(
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, kDepthFormat);

#if __APPLE__
  const auto dynamic_rendering_cmds =
      core::vulkan::LoadDynamicRenderingCommands(context.logical_device);
  const PFN_vkCmdBeginRendering vkCmdBeginRendering = dynamic_rendering_cmds.vkCmdBeginRendering;
  const PFN_vkCmdEndRendering vkCmdEndRendering = dynamic_rendering_cmds.vkCmdEndRendering;
#endif

  core::vulkan::VulkanCommandBuffer command_buffer(&context);
  core::vulkan::VulkanSemaphore image_available_semaphore(&context);
  core::vulkan::VulkanSemaphore render_finished_semaphore(&context);
  core::vulkan::VulkanFence in_flight_fence(&context);

  core::vulkan::DynamicRenderingInfo dynamic_rendering_info{};
  dynamic_rendering_info.color_formats = {swap_chain->swapchain_image_format};
  dynamic_rendering_info.depth_format = kDepthFormat;

  auto floor = std::make_unique<core::RenderFloor>(&context, dynamic_rendering_info);
  auto cube = std::make_unique<core::RenderCube>(&context, dynamic_rendering_info);
  floor->Init();
  cube->Init();

  core::PhysicsBody body;
  auto last_time = std::chrono::high_resolution_clock::now();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, true);
    }

    const auto now = std::chrono::high_resolution_clock::now();
    const float dt =
        std::chrono::duration<float, std::chrono::seconds::period>(now - last_time).count();
    last_time = now;

    // Fixed-step the physics so visual behaviour is independent of FPS.
    constexpr float kFixedDt = 1.0f / 240.0f;
    static float accumulator = 0.0f;
    accumulator = std::min(accumulator + dt, 0.25f);
    while (accumulator >= kFixedDt) {
      body.Step(kFixedDt);
      accumulator -= kFixedDt;
    }

    vkWaitForFences(context.logical_device, 1, &(in_flight_fence.fence), VK_TRUE, UINT64_MAX);
    in_flight_fence.Reset();

    uint32_t image_index;
    vkAcquireNextImageKHR(context.logical_device, swap_chain->swapchain, UINT64_MAX,
                          image_available_semaphore.semaphore, VK_NULL_HANDLE, &image_index);

    const glm::mat4 view = glm::lookAt(glm::vec3(6.0f, 4.5f, 8.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 project =
        glm::perspective(glm::radians(45.0f),
                         static_cast<float>(swap_chain->swapchain_extent.width) /
                             static_cast<float>(swap_chain->swapchain_extent.height),
                         0.1f, 100.0f);

    floor->UpdateUniformBuffer(view, project);
    cube->UpdateUniformBuffer(body.ModelMatrix(), view, project);

    command_buffer.Reset();
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(command_buffer.buffer(), &begin_info));

    swap_chain->TransitionImageLayout(command_buffer.buffer(), image_index,
                                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo color_attachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swap_chain->swapchain_image_views[image_index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {{{0.08f, 0.10f, 0.14f, 1.0f}}}};
    VkRenderingAttachmentInfo depth_attachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = depth_image.image_view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = {.depthStencil = {1.0f, 0}}};
    VkRenderingInfo rendering_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {.offset = {0, 0}, .extent = swap_chain->swapchain_extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = &depth_attachment};

    vkCmdBeginRendering(command_buffer.buffer(), &rendering_info);
    floor->Render(command_buffer.buffer(), swap_chain->swapchain_extent);
    cube->Render(command_buffer.buffer(), swap_chain->swapchain_extent);
    vkCmdEndRendering(command_buffer.buffer());

    swap_chain->TransitionImageLayout(command_buffer.buffer(), image_index,
                                      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore wait_semaphores[] = {image_available_semaphore.semaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    VkSemaphore signal_semaphores[] = {render_finished_semaphore.semaphore};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    command_buffer.Submit(in_flight_fence.fence, submit_info);

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
