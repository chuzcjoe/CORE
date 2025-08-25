#include "VulkanImage.h"

namespace core {
namespace vulkan {

VulkanImage::VulkanImage(VulkanContext* context, const uint32_t width, const uint32_t height,
                         const VkFormat format, const VkImageUsageFlags usage,
                         const VkImageAspectFlags aspect, const VkMemoryPropertyFlags properties)
    : context_(context), image_width_(width), image_height_(height) {
  VkImageCreateInfo image_info{};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = image_width_;
  image_info.extent.height = image_height_;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = format;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.usage = usage;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.flags = 0;  // Optional

  VK_CHECK(vkCreateImage(context_->logical_device, &image_info, nullptr, &image_));

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(context_->logical_device, image_, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      context_->FindMemoryType(mem_requirements.memoryTypeBits, properties);

  VK_CHECK(vkAllocateMemory(context_->logical_device, &alloc_info, nullptr, &image_memory_));

  vkBindImageMemory(context_->logical_device, image_, image_memory_, 0);

  // Create image view
  VkImageViewCreateInfo view_info{};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image_;
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

  VK_CHECK(vkCreateImageView(context_->logical_device, &view_info, nullptr, &image_view_));
}

VulkanImage::~VulkanImage() {
  if (image_view_ != VK_NULL_HANDLE) {
    vkDestroyImageView(context_->logical_device, image_view_, nullptr);
  }
  if (image_ != VK_NULL_HANDLE) {
    vkDestroyImage(context_->logical_device, image_, nullptr);
  }
  if (image_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(context_->logical_device, image_memory_, nullptr);
  }
}

}  // namespace vulkan
}  // namespace core