#pragma once

#include <vulkan/vulkan.h>

#include <functional>

#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanBuffer {
 public:
  VulkanBuffer(VulkanContext* context, const VkDeviceSize size, const VkBufferUsageFlags usage,
               const VkMemoryPropertyFlags properties);
  ~VulkanBuffer();

  void MapData(const std::function<void(void*)>& func);

 private:
  VulkanContext* context_;
  VkDeviceSize buffer_size_;
  VkMemoryPropertyFlags memory_properties_;

  VkBuffer buffer_;
  VkDeviceMemory buffer_memory_;
};

}  // namespace vulkan
}  // namespace core