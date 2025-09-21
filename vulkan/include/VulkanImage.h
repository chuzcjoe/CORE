#pragma once

#include <vulkan/vulkan.h>

#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanImage {
 public:
  VulkanImage() = default;
  VulkanImage(VulkanContext* context, const uint32_t width, const uint32_t height,
              const VkFormat format, const VkImageUsageFlags usage, const VkImageAspectFlags aspect,
              const VkMemoryPropertyFlags properties);
  ~VulkanImage();

  VulkanImage& operator=(VulkanImage&&);

  // void CreateTextureImage(const std::string& image_path);

 private:
  VulkanContext* context_;
  uint32_t image_width_;
  uint32_t image_height_;

  VkImage image_;
  VkImageView image_view_;
  VkDeviceMemory image_memory_;
};

}  // namespace vulkan
}  // namespace core