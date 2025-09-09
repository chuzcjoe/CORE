#include "VulkanSwapChain.h"

namespace core {
namespace vulkan {

VulkanSwapChain::VulkanSwapChain(VulkanContext* context, VkSurfaceKHR surface)
    : context_(context), surface_(surface) {
  swapchain_support_details_ = QuerySwapChainSupport(context_->physical_device);
  surface_format_ = ChooseSwapSurfaceFormat(swapchain_support_details_.formats);
  present_mode_ = ChooseSwapPresentMode(swapchain_support_details_.present_modes);
  swapchain_extent_ = ChooseSwapExtent(swapchain_support_details_.capabilities);

  uint32_t image_count = swapchain_support_details_.capabilities.minImageCount + 1;
  if (swapchain_support_details_.capabilities.maxImageCount > 0 &&
      image_count > swapchain_support_details_.capabilities.maxImageCount) {
    image_count = swapchain_support_details_.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface_;

  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format_.format;
  create_info.imageColorSpace = surface_format_.colorSpace;
  create_info.imageExtent = swapchain_extent_;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = context_->GetQueueFamilyIndices();
  uint32_t queue_family_indices[] = {indices.graphics_family.value(),
                                     indices.present_family.value()};

  if (indices.graphics_family != indices.present_family) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  create_info.preTransform = swapchain_support_details_.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode_;
  create_info.clipped = VK_TRUE;

  VK_CHECK(vkCreateSwapchainKHR(context_->logical_device, &create_info, nullptr, &swapchain_));

  vkGetSwapchainImagesKHR(context_->logical_device, swapchain_, &image_count, nullptr);
  swapchain_images_.resize(image_count);
  vkGetSwapchainImagesKHR(context_->logical_device, swapchain_, &image_count,
                          swapchain_images_.data());

  swapchain_image_format_ = surface_format_.format;
}

VulkanSwapChain::~VulkanSwapChain() {
  vkDestroySwapchainKHR(context_->logical_device, swapchain_, nullptr);
}

SwapChainSupportDetails VulkanSwapChain::QuerySwapChainSupport(VkPhysicalDevice device) {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
  if (format_count != 0) {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
  }

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
  if (present_mode_count != 0) {
    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count,
                                              details.present_modes.data());
  }
  return details;
}

VkSurfaceFormatKHR VulkanSwapChain::ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& formats) {
  for (const auto& format : formats) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }
  return formats[0];
}

VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& present_modes) {
  for (const auto& mode : present_modes) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return mode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
#if defined(__APPLE__)
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);

    VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width,
                                     capabilities.maxImageExtent.width);
    actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                                      capabilities.maxImageExtent.height);

    return actual_extent;
#endif
  }
}

}  // namespace vulkan
}  // namespace core