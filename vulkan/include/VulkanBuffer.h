#pragma once

#include <functional>
#include <iostream>

#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanSync.h"

namespace core {
namespace vulkan {

class VulkanBuffer {
 public:
  // VulkanBuffer() = delete;  // Buffers must be explicitly initialized
  VulkanBuffer() = default;
  VulkanBuffer(VulkanContext* context, const VkDeviceSize size, const VkBufferUsageFlags usage,
               const VkMemoryPropertyFlags properties);
  ~VulkanBuffer();

  VulkanBuffer& operator=(VulkanBuffer&&);

  void CopyBuffer(VulkanBuffer& dst_buffer);

  void MapData(const std::function<void(void*)>& func);

  VkDeviceSize Size() const { return buffer_size_; }

  VkBuffer buffer;

 private:
  VulkanContext* context_;
  VkDeviceSize buffer_size_;
  VkMemoryPropertyFlags memory_properties_;
  VkDeviceMemory buffer_memory_;
};

}  // namespace vulkan
}  // namespace core