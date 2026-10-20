#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include "Status.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanBuffer;

class VulkanImage {
 public:
  VulkanImage(const VulkanImage&) = delete;
  VulkanImage& operator=(const VulkanImage&) = delete;
  VulkanImage() = default;

  static Result<std::unique_ptr<VulkanImage>> Create(
      VulkanContext* context, const uint32_t width, const uint32_t height,
      const VkFormat format, const VkImageUsageFlags usage,
      const VkImageAspectFlags aspect, const VkMemoryPropertyFlags properties,
      const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
      const uint32_t mip_levels = 1,
      const VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
      const VkImageCreateFlags flags = 0, const uint32_t layers = 1);
  Status Init(VulkanContext* context, const uint32_t width,
              const uint32_t height, const VkFormat format,
              const VkImageUsageFlags usage, const VkImageAspectFlags aspect,
              const VkMemoryPropertyFlags properties,
              const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
              const uint32_t mip_levels = 1,
              const VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
              const VkImageCreateFlags flags = 0, const uint32_t layers = 1);
  ~VulkanImage();

  VulkanImage& operator=(VulkanImage&&);

  Status TransitionImageLayout(const VkImageLayout old_layout,
                               const VkImageLayout new_layout,
                               [[maybe_unused]] const VkFormat format,
                               const uint32_t mip_levels = 1,
                               const uint32_t layers = 1);

  Status TransitionDepthImageLayout(const VkImageLayout old_layout,
                                    const VkImageLayout new_layout,
                                    [[maybe_unused]] const VkFormat format);

  Status GenerateMipmaps();

  // void CreateTextureImage(const std::string& image_path);

 private:
  VulkanContext* context_ = nullptr;
  VkFormat image_format_ = VK_FORMAT_UNDEFINED;
  uint32_t mip_levels_ = 1;
  uint32_t array_layers_ = 1;
  VkSampleCountFlagBits samples_ = VK_SAMPLE_COUNT_1_BIT;

 public:
  uint32_t image_width = 0;
  uint32_t image_height = 0;

  VkImage image = VK_NULL_HANDLE;
  VkImageView image_view = VK_NULL_HANDLE;
  VkDeviceMemory image_memory = VK_NULL_HANDLE;

 private:
  bool initialized_ = false;
};

}  // namespace vulkan
}  // namespace core
