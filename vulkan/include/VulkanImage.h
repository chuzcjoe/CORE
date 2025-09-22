#pragma once

#include <vulkan/vulkan.h>

#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanBuffer;

class VulkanImage {
 public:
  VulkanImage() = default;
  VulkanImage(VulkanContext* context, const uint32_t width, const uint32_t height,
              const VkFormat format, const VkImageUsageFlags usage, const VkImageAspectFlags aspect,
              const VkMemoryPropertyFlags properties);
  ~VulkanImage();

  VulkanImage& operator=(VulkanImage&&);

  void TransitionImageLayout(const VkImageLayout old_layout, const VkImageLayout new_layout,
                             [[maybe_unused]] const VkFormat format);

  // void CreateTextureImage(const std::string& image_path);

 private:
  VulkanContext* context_;

 public:
  uint32_t image_width;
  uint32_t image_height;

  VkImage image;
  VkImageView image_view;
  VkDeviceMemory image_memory;
};

}  // namespace vulkan
}  // namespace core