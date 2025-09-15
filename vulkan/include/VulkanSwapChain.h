#pragma once

#if defined(__APPLE__)
#include <GLFW/glfw3.h>
#endif

#include "VulkanContext.h"
#include "VulkanRenderPass.h"

namespace core {
namespace vulkan {

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

class VulkanSwapChain {
 public:
  VulkanSwapChain() = delete;
  explicit VulkanSwapChain(VulkanContext* context, VkSurfaceKHR surface);
  ~VulkanSwapChain();

  void CreateFrameBuffers(VulkanRenderPass& render_pass);

  VkExtent2D swapchain_extent;
  std::vector<VkFramebuffer> swapchain_framebuffers;
  VkSwapchainKHR swapchain;
  VkFormat swapchain_image_format;

 private:
  SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
  VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
  VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& present_modes);
  VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

  void CreateImageViews();

  VulkanContext* context_;
  SwapChainSupportDetails swapchain_support_details_;
  VkSurfaceKHR surface_;
  VkSurfaceFormatKHR surface_format_;
  VkPresentModeKHR present_mode_;
  std::vector<VkImage> swapchain_images_;

  std::vector<VkImageView> swapchain_image_views_;

#if defined(__APPLE__)
  GLFWwindow* window_;
#endif
};

}  // namespace vulkan
}  // namespace core