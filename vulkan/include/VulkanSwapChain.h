#pragma once

#if defined(__APPLE__)
#include <GLFW/glfw3.h>
#endif

#include "VulkanContext.h"

namespace core {
namespace vulkan {

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

class VulkanSwapChain {
 public:
  explicit VulkanSwapChain(VulkanContext* context, VkSurfaceKHR surface, VkRenderPass render_pass);
  ~VulkanSwapChain();

 private:
  SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
  VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
  VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& present_modes);
  VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

  void CreateImageViews();
  void CreateFrameBuffers();

  VulkanContext* context_;
  SwapChainSupportDetails swapchain_support_details_;
  VkSurfaceKHR surface_;
  VkRenderPass render_pass_;
  VkSurfaceFormatKHR surface_format_;
  VkPresentModeKHR present_mode_;
  VkExtent2D swapchain_extent_;
  VkSwapchainKHR swapchain_;
  VkFormat swapchain_image_format_;
  std::vector<VkImage> swapchain_images_;

  std::vector<VkImageView> swapchain_image_views_;
  std::vector<VkFramebuffer> swapchain_framebuffers_;

#if defined(__APPLE__)
  GLFWwindow* window_;
#endif
};

}  // namespace vulkan
}  // namespace core