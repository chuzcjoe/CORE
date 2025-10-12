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
              const VkMemoryPropertyFlags properties,
              const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);
  ~VulkanImage();

  VulkanImage& operator=(VulkanImage&&);

  void TransitionImageLayout(const VkImageLayout old_layout, const VkImageLayout new_layout,
                             [[maybe_unused]] const VkFormat format);

  void TransitionDepthImageLayout(const VkImageLayout old_layout, const VkImageLayout new_layout,
                                  [[maybe_unused]] const VkFormat format);

  // void CreateTextureImage(const std::string& image_path);

 private:
  VulkanContext* context_ = nullptr;

 public:
  uint32_t image_width;
  uint32_t image_height;

  VkImage image = VK_NULL_HANDLE;
  VkImageView image_view = VK_NULL_HANDLE;
  VkDeviceMemory image_memory = VK_NULL_HANDLE;
};

}  // namespace vulkan
}  // namespace core