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
  explicit VulkanSwapChain(VulkanContext* context, VkSurfaceKHR surface,
                           const bool enable_depth_buffer = false);
  ~VulkanSwapChain();

  void UnInit();

  void CreateFrameBuffers(VulkanRenderPass& render_pass);

  VkExtent2D swapchain_extent;
  std::vector<VkFramebuffer> swapchain_framebuffers;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  VkFormat swapchain_image_format;

 private:
  SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
  VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
  VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& present_modes);
  VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

  void CreateImageViews();
  void CreateDepthResources(VkFormat depth_format);

  VulkanContext* context_ = nullptr;
  SwapChainSupportDetails swapchain_support_details_;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkSurfaceFormatKHR surface_format_;
  VkPresentModeKHR present_mode_;
  std::vector<VkImage> swapchain_images_;

  std::vector<VkImageView> swapchain_image_views_;
  bool enable_depth_buffer_ = false;
  core::vulkan::VulkanImage depth_image_;

#if defined(__APPLE__)
  GLFWwindow* window_;
#endif
};

}  // namespace vulkan
}  // namespace core