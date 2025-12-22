#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "GraphicModel.h"
#include "VulkanCamera.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSwapChain.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

void process_inputs(GLFWwindow* window);
void mouse_callback([[maybe_unused]] GLFWwindow* window, double xpos, double ypos);

const uint32_t kWidth = 1000;
const uint32_t kHeight = 1000;
const bool kEnableDepthBuffer = true;
const std::string kModelPath = "./examples/data/viking_room.obj";
const std::string kTexturePath = "./examples/data/viking_room.png";
const glm::vec3 kCameraPos = glm::vec3(0.0f, 1.5f, 5.0f);
const glm::vec3 kCameraFront = glm::vec3(0.0f, 0.0f, -1.0f);  // look toward -Z
const glm::vec3 kCameraUp = glm::vec3(0.0f, 1.0f, 0.0f);      // Y-up
const float kCameraSpeed = 0.03f;
const float kMouseSensitivity = 0.1f;
float laxt_x = kWidth / 2.0f;
float last_y = kHeight / 2.0f;
bool first_mouse = true;

std::unique_ptr<core::vulkan::VulkanCamera> camera =
    std::make_unique<core::vulkan::VulkanCamera>(kCameraPos, kCameraFront, kCameraUp, kCameraSpeed);

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

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(window, mouse_callback);

  core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Graphics;
  core::vulkan::VulkanContext context(true, queue_family_type, nullptr);
  std::unique_ptr<core::vulkan::VulkanSwapChain> swap_chain;
  if (glfwCreateWindowSurface(context.instance, window, nullptr, &window_surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface");
  } else {
    context.Init(window_surface);
  }

  if (window_surface != VK_NULL_HANDLE) {
    swap_chain = std::make_unique<core::vulkan::VulkanSwapChain>(&context, window_surface,
                                                                 kEnableDepthBuffer);
  }

  core::vulkan::VulkanCommandBuffer command_buffer(&context);
  core::vulkan::VulkanFence fence(&context);
  core::vulkan::VulkanSemaphore image_available_semaphore(&context);
  core::vulkan::VulkanSemaphore render_finished_semaphore(&context);
  core::vulkan::VulkanFence in_flight_fence(&context);
  core::vulkan::VulkanRenderPass render_pass(&context, swap_chain->swapchain_image_format,
                                             kEnableDepthBuffer);  // enable depth buffer
  std::unique_ptr<core::GraphicModel> model =
      std::make_unique<core::GraphicModel>(&context, render_pass);
  model->Init(kTexturePath, kModelPath);
  swap_chain->CreateFrameBuffers(render_pass);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    process_inputs(window);
    // draw process
    vkWaitForFences(context.logical_device, 1, &(in_flight_fence.fence), VK_TRUE, UINT64_MAX);
    in_flight_fence.Reset();
    uint32_t image_index;
    vkAcquireNextImageKHR(context.logical_device, swap_chain->swapchain, UINT64_MAX,
                          image_available_semaphore.semaphore, VK_NULL_HANDLE, &image_index);
    const auto camera_view = camera->GetViewMatrix();
    model->UpdateUniformBuffer(swap_chain->swapchain_extent.width,
                               swap_chain->swapchain_extent.height, camera_view);
    // ========== Command buffer begin ==========
    command_buffer.Reset();
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(command_buffer.buffer(), &begin_info));
    VkRenderPassBeginInfo renderpass_info{};
    renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_info.renderPass = render_pass.GetRenderPass();
    renderpass_info.framebuffer = swap_chain->swapchain_framebuffers[image_index];
    renderpass_info.renderArea.offset = {0, 0};
    renderpass_info.renderArea.extent = swap_chain->swapchain_extent;
    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};
    renderpass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    renderpass_info.pClearValues = clear_values.data();
    vkCmdBeginRenderPass(command_buffer.buffer(), &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
    model->Render(command_buffer.buffer(), swap_chain->swapchain_extent);
    vkCmdEndRenderPass(command_buffer.buffer());

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
  swap_chain->UnInit();  // ImageViews and FrameBuffers need to be released before context release
  vkDestroySurfaceKHR(context.instance, window_surface,
                      nullptr);  // Surface needs to be released before context release
  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}

void process_inputs(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera->ProcessKeyboard(core::vulkan::CameraMovement::FORWARD);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera->ProcessKeyboard(core::vulkan::CameraMovement::BACKWARD);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera->ProcessKeyboard(core::vulkan::CameraMovement::LEFT);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera->ProcessKeyboard(core::vulkan::CameraMovement::RIGHT);
}

void mouse_callback([[maybe_unused]] GLFWwindow* window, double xpos, double ypos) {
  if (first_mouse) {
    laxt_x = xpos;
    last_y = ypos;
    first_mouse = false;
  }
  float xoffset = xpos - laxt_x;
  float yoffset = last_y - ypos;  // reversed since y-coordinates go from bottom to top
  laxt_x = xpos;
  last_y = ypos;

  xoffset *= kMouseSensitivity;
  yoffset *= kMouseSensitivity;

  camera->yaw += xoffset;
  camera->pitch += yoffset;

  if (camera->pitch > 89.0f) camera->pitch = 89.0f;
  if (camera->pitch < -89.0f) camera->pitch = -89.0f;

  glm::vec3 front;
  front.x = std::cos(glm::radians(camera->yaw)) * std::cos(glm::radians(camera->pitch));
  front.y = std::sin(glm::radians(camera->pitch));
  front.z = std::sin(glm::radians(camera->yaw)) * std::cos(glm::radians(camera->pitch));
  camera->camera_front = glm::normalize(front);
}