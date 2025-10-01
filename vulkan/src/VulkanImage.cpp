#include "VulkanImage.h"

#include "VulkanBuffer.h"

namespace core {
namespace vulkan {

VulkanImage::VulkanImage(VulkanContext* context, const uint32_t width, const uint32_t height,
                         const VkFormat format, const VkImageUsageFlags usage,
                         const VkImageAspectFlags aspect, const VkMemoryPropertyFlags properties,
                         const VkImageTiling tiling)
    : context_(context), image_width(width), image_height(height) {
  VkImageCreateInfo image_info{};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = image_width;
  image_info.extent.height = image_height;
  image_info.extent.depth = 1;  // 2D image has depth 1
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = format;
  image_info.tiling = tiling;
  image_info.initialLayout =
      VK_IMAGE_LAYOUT_UNDEFINED;  // only two states: UNDEFINED and PREINITIALIZED
  image_info.usage = usage;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.flags = 0;  // Optional

  VK_CHECK(vkCreateImage(context_->logical_device, &image_info, nullptr, &image));

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(context_->logical_device, image, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      context_->FindMemoryType(mem_requirements.memoryTypeBits, properties);

  VK_CHECK(vkAllocateMemory(context_->logical_device, &alloc_info, nullptr, &image_memory));

  vkBindImageMemory(context_->logical_device, image, image_memory, 0);

  // Create image view
  VkImageViewCreateInfo view_info{};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = format;
  view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.subresourceRange.aspectMask = aspect;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  VK_CHECK(vkCreateImageView(context_->logical_device, &view_info, nullptr, &image_view));
}

VulkanImage::~VulkanImage() {
  if (image_view != VK_NULL_HANDLE) {
    vkDestroyImageView(context_->logical_device, image_view, nullptr);
  }
  if (image != VK_NULL_HANDLE) {
    vkDestroyImage(context_->logical_device, image, nullptr);
  }
  if (image_memory != VK_NULL_HANDLE) {
    vkFreeMemory(context_->logical_device, image_memory, nullptr);
  }
}

VulkanImage& VulkanImage::operator=(VulkanImage&& rhs) {
  if (image_view != VK_NULL_HANDLE) {
    vkDestroyImageView(context_->logical_device, image_view, nullptr);
  }
  if (image != VK_NULL_HANDLE) {
    vkDestroyImage(context_->logical_device, image, nullptr);
  }
  if (image_memory != VK_NULL_HANDLE) {
    vkFreeMemory(context_->logical_device, image_memory, nullptr);
  }

  context_ = rhs.context_;
  image = rhs.image;
  rhs.image = VK_NULL_HANDLE;

  image_view = rhs.image_view;
  rhs.image_view = VK_NULL_HANDLE;

  image_memory = rhs.image_memory;
  rhs.image_memory = VK_NULL_HANDLE;

  image_width = rhs.image_width;
  image_height = rhs.image_height;

  return *this;
}

void VulkanImage::TransitionImageLayout(const VkImageLayout old_layout,
                                        const VkImageLayout new_layout,
                                        [[maybe_unused]] const VkFormat format) {
  VulkanCommandBuffer command_buffer = VulkanCommandBuffer::BeginOneTimeCommands(context_);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags source_stage;
  VkPipelineStageFlags destination_stage;

  /**
   * case 1: undefined -> transfer destination
   * case 2: transfer destination -> shader read
   */
  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;  // Not a real stage within the graphics and
                                                         // compute pipelines. It is more of a
                                                         // pseudo-stage where transfers happen
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(command_buffer.buffer(), source_stage, destination_stage, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  command_buffer.EndOneTimeCommands();
}

void VulkanImage::TransitionDepthImageLayout(const VkImageLayout old_layout,
                                             const VkImageLayout new_layout,
                                             [[maybe_unused]] const VkFormat format) {
  VulkanCommandBuffer command_buffer = VulkanCommandBuffer::BeginOneTimeCommands(context_);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags source_stage;
  VkPipelineStageFlags destination_stage;

  if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(command_buffer.buffer(), source_stage, destination_stage, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  command_buffer.EndOneTimeCommands();
}

}  // namespace vulkan
}  // namespace core