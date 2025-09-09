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
  explicit VulkanSwapChain(VulkanContext* context, VkSurfaceKHR surface);
  ~VulkanSwapChain();

 private:
  SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
  VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
  VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& present_modes);
  VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

  VulkanContext* context_;
  SwapChainSupportDetails swapchain_support_details_;
  VkSurfaceKHR surface_;
  VkSurfaceFormatKHR surface_format_;
  VkPresentModeKHR present_mode_;
  VkExtent2D extent_;
  VkSwapchainKHR swapchain_;

#if defined(__APPLE__)
  GLFWwindow* window_;
#endif
};

}  // namespace vulkan
}  // namespace core