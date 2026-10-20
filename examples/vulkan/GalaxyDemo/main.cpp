#include "Status.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include "RenderGalaxy.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDynamicRendering.h"
#include "VulkanSwapChain.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

// Renders a procedural spiral galaxy with ~50k stars rotating with differential angular velocity.
const uint32_t kWidth = 1280;
const uint32_t kHeight = 800;
const uint32_t kStarCount = 60000;

core::Status RunApplication() {
  VkSurfaceKHR window_surface = VK_NULL_HANDLE;
  GLFWwindow* window;
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(kWidth, kHeight, "Galaxy", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return core::Status(core::ErrorCode::kBackend, "failed to create window");
  }
  const auto close_window = [](GLFWwindow* value) {
    glfwDestroyWindow(value);
    glfwTerminate();
  };
  const std::unique_ptr<GLFWwindow, decltype(close_window)> window_owner(
      window, close_window);

  core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Graphics;

  auto context_result =
      core::vulkan::VulkanContext::Create(true, queue_family_type, nullptr);
  if (!context_result.ok()) {
    return context_result.status();
  }
  std::unique_ptr<core::vulkan::VulkanContext> context_owner =
      std::move(*context_result);
  core::vulkan::VulkanContext& context = *context_owner;

  if (glfwCreateWindowSurface(context.instance, window, nullptr, &window_surface) != VK_SUCCESS) {
    return core::Status(core::ErrorCode::kBackend,
                        "failed to create window surface");
  }
  const auto destroy_surface = [&context](VkSurfaceKHR value) {
    vkDestroySurfaceKHR(context.instance, value, nullptr);
  };
  const std::unique_ptr<std::remove_pointer_t<VkSurfaceKHR>,
                        decltype(destroy_surface)>
      surface_owner(window_surface, destroy_surface);
  std::unique_ptr<core::vulkan::VulkanSwapChain> swap_chain;

  {
    const core::Status status = context.Init(window_surface);
    if (!status.ok()) {
      return status;
    }
  }
  if (window_surface != VK_NULL_HANDLE) {
    auto swap_chain_creation =
        core::vulkan::VulkanSwapChain::Create(&context, window_surface);
    if (!swap_chain_creation.ok()) {
      return swap_chain_creation.status();
    }
    swap_chain = std::move(*swap_chain_creation);
  }

  auto command_buffer_result =
      core::vulkan::VulkanCommandBuffer::Create(&context);
  if (!command_buffer_result.ok()) {
    return command_buffer_result.status();
  }
  std::unique_ptr<core::vulkan::VulkanCommandBuffer> command_buffer_owner =
      std::move(*command_buffer_result);
  core::vulkan::VulkanCommandBuffer& command_buffer = *command_buffer_owner;

  auto dynamic_rendering_result =
      core::vulkan::VulkanDynamicRendering::Create(&context);
  if (!dynamic_rendering_result.ok()) {
    return dynamic_rendering_result.status();
  }
  std::unique_ptr<core::vulkan::VulkanDynamicRendering>
      dynamic_rendering_owner = std::move(*dynamic_rendering_result);
  core::vulkan::VulkanDynamicRendering& dynamic_rendering =
      *dynamic_rendering_owner;

  auto image_available_semaphore_result =
      core::vulkan::VulkanSemaphore::Create(&context);
  if (!image_available_semaphore_result.ok()) {
    return image_available_semaphore_result.status();
  }
  std::unique_ptr<core::vulkan::VulkanSemaphore>
      image_available_semaphore_owner =
          std::move(*image_available_semaphore_result);
  core::vulkan::VulkanSemaphore& image_available_semaphore =
      *image_available_semaphore_owner;
  std::vector<std::unique_ptr<core::vulkan::VulkanSemaphore>> render_finished_semaphores;

  auto in_flight_fence_result = core::vulkan::VulkanFence::Create(&context);
  if (!in_flight_fence_result.ok()) {
    return in_flight_fence_result.status();
  }
  std::unique_ptr<core::vulkan::VulkanFence> in_flight_fence_owner =
      std::move(*in_flight_fence_result);
  core::vulkan::VulkanFence& in_flight_fence = *in_flight_fence_owner;
  core::vulkan::DynamicRenderingInfo dynamic_rendering_info{
      .color_formats = {swap_chain->swapchain_image_format}};
  auto galaxy = std::make_unique<core::RenderGalaxy>(&context, dynamic_rendering_info, kStarCount);
  const VkClearValue clear_value{
      .color =
          {
              .float32 = {0.01f, 0.005f, 0.02f, 1.0f},
          },
  };
  for (size_t i = 0; i < swap_chain->swapchain_images.size(); ++i) {
    auto created_resource_4 = core::vulkan::VulkanSemaphore::Create(&context);
    if (!created_resource_4.ok()) {
      return created_resource_4.status();
    }
    render_finished_semaphores.push_back(std::move(*created_resource_4));
  }

  {
    const core::Status status = galaxy->Init();
    if (!status.ok()) {
      return status;
    }
  }

  const auto wait_for_device = [](core::vulkan::VulkanContext* value) {
    static_cast<void>(vkDeviceWaitIdle(value->logical_device));
  };
  const std::unique_ptr<core::vulkan::VulkanContext, decltype(wait_for_device)>
      device_idle_guard(&context, wait_for_device);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    vkWaitForFences(context.logical_device, 1, &(in_flight_fence.fence), VK_TRUE, UINT64_MAX);
    {
      const core::Status status = in_flight_fence.Reset();
      if (!status.ok()) {
        return status;
      }
    }

    uint32_t image_index;
    vkAcquireNextImageKHR(context.logical_device, swap_chain->swapchain, UINT64_MAX,
                          image_available_semaphore.semaphore, VK_NULL_HANDLE, &image_index);

    {
      const core::Status status =
          galaxy->UpdateUniformBuffer(swap_chain->swapchain_extent.width,
                                      swap_chain->swapchain_extent.height);
      if (!status.ok()) {
        return status;
      }
    }

    {
      const core::Status status = command_buffer.BeginCommandBufferWithReset();
      if (!status.ok()) {
        return status;
      }
    }

    {
      const core::Status status = swap_chain->TransitionImageLayout(
          command_buffer.buffer(), image_index,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
      if (!status.ok()) {
        return status;
      }
    }

    dynamic_rendering.BeginDynamicRendering(command_buffer.buffer(),
                                            swap_chain->swapchain_image_views[image_index],
                                            swap_chain->swapchain_extent, clear_value);
    galaxy->Render(command_buffer.buffer(), swap_chain->swapchain_extent);
    dynamic_rendering.EndDynamicRendering(command_buffer.buffer());

    {
      const core::Status status = swap_chain->TransitionImageLayout(
          command_buffer.buffer(), image_index,
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
      if (!status.ok()) {
        return status;
      }
    }

    {
      const core::Status status = command_buffer.Submit(
          in_flight_fence.fence,
          core::vulkan::SubmitSyncInfo{
              .wait_semaphores = {image_available_semaphore.semaphore},
              .wait_stage_masks =
                  {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
              .signal_semaphores =
                  {render_finished_semaphores[image_index]->semaphore},
          });
      if (!status.ok()) {
        return status;
      }
    }

    const VkResult present_result =
        swap_chain->Present(image_index, render_finished_semaphores[image_index]->semaphore);
    if (present_result != VK_SUCCESS && present_result != VK_SUBOPTIMAL_KHR) {
      {
        const VkResult error = present_result;
        if (error != VK_SUCCESS) {
          return core::Status(core::ErrorCode::kBackend,
                              core::vulkan::VkErrorMessages(error), "Vulkan",
                              error);
        }
      }
    }
  }
  vkDeviceWaitIdle(context.logical_device);

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
