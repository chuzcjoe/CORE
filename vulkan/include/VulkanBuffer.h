#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <iostream>

#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanSync.h"

namespace core {
namespace vulkan {

class VulkanImage;

class VulkanBuffer {
 public:
  // VulkanBuffer() = delete;  // Buffers must be explicitly initialized
  VulkanBuffer() = default;
  VulkanBuffer(VulkanContext* context, const VkDeviceSize size, const VkBufferUsageFlags usage,
               const VkMemoryPropertyFlags properties);
  ~VulkanBuffer();

  VulkanBuffer& operator=(VulkanBuffer&&);

  void CopyToBuffer(VulkanBuffer& dst_buffer);

  void CopyToImage(VulkanImage& dst_image, const uint32_t width, const uint32_t height);

  void MapData(const std::function<void(void*)>& func);

  VkDeviceSize Size() const { return buffer_size_; }

  VkBuffer buffer;

  // TODO: keep them public
 private:
  VulkanContext* context_;
  VkDeviceSize buffer_size_;
  VkMemoryPropertyFlags memory_properties_;
  VkDeviceMemory buffer_memory_;
};

}  // namespace vulkan
}  // namespace core