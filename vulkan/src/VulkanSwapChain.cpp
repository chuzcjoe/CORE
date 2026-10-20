#include "VulkanSwapChain.h"

#include "Status.h"
#include "VulkanCommandBuffer.h"

namespace core {
namespace vulkan {

Result<std::unique_ptr<VulkanSwapChain>> VulkanSwapChain::Create(
    VulkanContext* context, VkSurfaceKHR surface,
    const bool enable_depth_buffer) {
  std::unique_ptr<VulkanSwapChain> object(new VulkanSwapChain());
  const Status status = object->Init(context, surface, enable_depth_buffer);
  if (!status.ok()) {
    return Result<std::unique_ptr<VulkanSwapChain>>(status);
  }
  return Result<std::unique_ptr<VulkanSwapChain>>(std::move(object));
}

Status VulkanSwapChain::Init(VulkanContext* context, VkSurfaceKHR surface,
                             const bool enable_depth_buffer) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanSwapChain initialization is single-use");
  }
  initialized_ = true;
  context_ = context;
  if (context_ == nullptr || !context_->is_initialized()) {
    return Status(ErrorCode::kInvalidState, "A live Vulkan device is required");
  }

  surface_ = surface;
  enable_depth_buffer_ = enable_depth_buffer;

  auto support = QuerySwapChainSupport(context_->physical_device);
  if (!support.ok()) {
    return support.status();
  }
  swapchain_support_details_ = std::move(*support);
  if (swapchain_support_details_.formats.empty() ||
      swapchain_support_details_.present_modes.empty()) {
    return Status(ErrorCode::kUnsupported,
                  "Surface has no supported formats or presentation modes");
  }
  surface_format_ = ChooseSwapSurfaceFormat(swapchain_support_details_.formats);
  present_mode_ = ChooseSwapPresentMode(swapchain_support_details_.present_modes);
  swapchain_extent = ChooseSwapExtent(swapchain_support_details_.capabilities);

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
  create_info.imageExtent = swapchain_extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = context_->GetQueueFamilyIndices();
  if (!indices.graphics_family.has_value() ||
      !indices.present_family.has_value()) {
    return Status(ErrorCode::kInvalidState,
                  "Swapchains require graphics and present queues");
  }
  uint32_t queue_family_indices[] = {*indices.graphics_family,
                                     *indices.present_family};

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

  VkResult result =
      vkCreateSwapchainKHR(context_->logical_device, &create_info, nullptr, &swapchain);
  if (result != VK_SUCCESS) {
    return Status(ErrorCode::kBackend, "create swap chain failed", "Vulkan",
                  result);
  }

  vkGetSwapchainImagesKHR(context_->logical_device, swapchain, &image_count, nullptr);
  swapchain_images.resize(image_count);
  swapchain_image_layouts_.resize(image_count, VK_IMAGE_LAYOUT_UNDEFINED);
  result = vkGetSwapchainImagesKHR(context_->logical_device, swapchain, &image_count,
                                   swapchain_images.data());
  if (result != VK_SUCCESS) {
    return Status(ErrorCode::kBackend, "get swap chain images failed", "Vulkan",
                  result);
  }
  swapchain_image_format = surface_format_.format;

  {
    const core::Status status = CreateImageViews();
    if (!status.ok()) {
      return status;
    }
  }

  return Status();
}

VulkanSwapChain::~VulkanSwapChain() {
  if (context_ == nullptr || context_->logical_device == VK_NULL_HANDLE) {
    return;
  }

  UnInit();
}

void VulkanSwapChain::UnInit() {
  if (context_ == nullptr || context_->logical_device == VK_NULL_HANDLE) {
    return;
  }
  for (auto framebuffer : swapchain_framebuffers) {
    vkDestroyFramebuffer(context_->logical_device, framebuffer, nullptr);
  }
  swapchain_framebuffers.clear();
  for (auto imageview : swapchain_image_views) {
    vkDestroyImageView(context_->logical_device, imageview, nullptr);
  }
  swapchain_image_views.clear();
  if (swapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(context_->logical_device, swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
  }
}

VkResult VulkanSwapChain::Present(uint32_t image_index, VkSemaphore wait_semaphore) const {
  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &wait_semaphore;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain;
  present_info.pImageIndices = &image_index;
  return vkQueuePresentKHR(context_->present_queue(), &present_info);
}

Result<SwapChainSupportDetails> VulkanSwapChain::QuerySwapChainSupport(
    VkPhysicalDevice device) {
  SwapChainSupportDetails details;

  {
    const VkResult error = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device, surface_, &details.capabilities);
    if (error != VK_SUCCESS) {
      return Result<SwapChainSupportDetails>(
          Status(ErrorCode::kBackend, VkErrorMessages(error), "Vulkan", error));
    }
  }

  uint32_t format_count;

  {
    const VkResult error = vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, surface_, &format_count, nullptr);
    if (error != VK_SUCCESS) {
      return Result<SwapChainSupportDetails>(
          Status(ErrorCode::kBackend, VkErrorMessages(error), "Vulkan", error));
    }
  }
  if (format_count != 0) {
    details.formats.resize(format_count);

    {
      const VkResult error = vkGetPhysicalDeviceSurfaceFormatsKHR(
          device, surface_, &format_count, details.formats.data());
      if (error != VK_SUCCESS) {
        return Result<SwapChainSupportDetails>(Status(
            ErrorCode::kBackend, VkErrorMessages(error), "Vulkan", error));
      }
    }
  }

  uint32_t present_mode_count;

  {
    const VkResult error = vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface_, &present_mode_count, nullptr);
    if (error != VK_SUCCESS) {
      return Result<SwapChainSupportDetails>(
          Status(ErrorCode::kBackend, VkErrorMessages(error), "Vulkan", error));
    }
  }
  if (present_mode_count != 0) {
    details.present_modes.resize(present_mode_count);

    {
      const VkResult error = vkGetPhysicalDeviceSurfacePresentModesKHR(
          device, surface_, &present_mode_count, details.present_modes.data());
      if (error != VK_SUCCESS) {
        return Result<SwapChainSupportDetails>(Status(
            ErrorCode::kBackend, VkErrorMessages(error), "Vulkan", error));
      }
    }
  }
  return Result<SwapChainSupportDetails>(std::move(details));
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
  return capabilities.currentExtent;
}

Status VulkanSwapChain::CreateImageViews() {
  if (context_ == nullptr || swapchain == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanSwapChain requires successful initialization");
  }

  swapchain_image_views.resize(swapchain_images.size());
  for (size_t i = 0; i < swapchain_images.size(); ++i) {
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = swapchain_images[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = swapchain_image_format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    {
      const VkResult result =
          vkCreateImageView(context_->logical_device, &create_info, nullptr,
                            &swapchain_image_views[i]);
      if (result != VK_SUCCESS) {
        return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                      result);
      }
    }
  }

  return Status();
}

Status VulkanSwapChain::CreateDepthResources(VkFormat depth_format) {
  if (context_ == nullptr || swapchain == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanSwapChain requires successful initialization");
  }

  {
    const core::Status status = depth_image_.Init(
        context_, swapchain_extent.width, swapchain_extent.height, depth_format,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL);
    if (!status.ok()) {
      return status;
    }
  }

  {
    const core::Status status = depth_image_.TransitionDepthImageLayout(
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, depth_format);
    if (!status.ok()) {
      return status;
    }
  }

  return Status();
}

Status VulkanSwapChain::CreateFrameBuffers(VulkanRenderPass& render_pass) {
  if (context_ == nullptr || swapchain == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanSwapChain requires successful initialization");
  }

  if (enable_depth_buffer_) {
    {
      const core::Status status =
          CreateDepthResources(render_pass.depth_format);
      if (!status.ok()) {
        return status;
      }
    }
  }

  swapchain_framebuffers.resize(swapchain_image_views.size());

  for (size_t i = 0; i < swapchain_image_views.size(); ++i) {
    std::vector<VkImageView> attachments;
    attachments.push_back(swapchain_image_views[i]);
    if (enable_depth_buffer_) {
      attachments.push_back(depth_image_.image_view);
    }

    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass.GetRenderPass();
    framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebuffer_info.pAttachments = attachments.data();
    framebuffer_info.width = swapchain_extent.width;
    framebuffer_info.height = swapchain_extent.height;
    framebuffer_info.layers = 1;

    {
      const VkResult result =
          vkCreateFramebuffer(context_->logical_device, &framebuffer_info,
                              nullptr, &swapchain_framebuffers[i]);
      if (result != VK_SUCCESS) {
        return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                      result);
      }
    }
  }

  return Status();
}

Status VulkanSwapChain::TransitionImageLayout(VkCommandBuffer command_buffer,
                                              uint32_t image_index,
                                              VkImageLayout new_layout) {
  if (context_ == nullptr || swapchain == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanSwapChain requires successful initialization");
  }

  if (image_index >= swapchain_images.size()) {
    return Status(ErrorCode::kOutOfRange, "swapchain image index out of range");
  }

  VkImageLayout old_layout = swapchain_image_layouts_[image_index];

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = swapchain_images[image_index];
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR &&
             new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    source_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;
    source_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    destination_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  } else {
    return Status(ErrorCode::kInvalidArgument,
                  "unsupported swapchain image layout transition!");
  }

  swapchain_image_layouts_[image_index] = new_layout;

  vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr,
                       1, &barrier);

  return Status();
}

}  // namespace vulkan
}  // namespace core
