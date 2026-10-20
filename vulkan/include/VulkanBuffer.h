#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <iostream>
#include <memory>
#include <span>

#include "Status.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanSync.h"

namespace core {
namespace vulkan {

class VulkanImage;
class VulkanBuffer;

struct BatchBufferCopy {
  const VulkanBuffer* source = nullptr;
  VulkanBuffer* destination = nullptr;
  VkDeviceSize source_offset = 0;
  VkDeviceSize destination_offset = 0;
  VkDeviceSize size = VK_WHOLE_SIZE;
};

class VulkanBuffer {
 public:
  VulkanBuffer(const VulkanBuffer&) = delete;
  VulkanBuffer& operator=(const VulkanBuffer&) = delete;
  VulkanBuffer() = default;

  static Result<std::unique_ptr<VulkanBuffer>> Create(
      VulkanContext* context, const VkDeviceSize size,
      const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties);
  Status Init(VulkanContext* context, const VkDeviceSize size,
              const VkBufferUsageFlags usage,
              const VkMemoryPropertyFlags properties);
  ~VulkanBuffer();

  VulkanBuffer& operator=(VulkanBuffer&&);

  Status CopyToBuffer(VulkanBuffer& dst_buffer);

  // Execute copies in order with one submission and fence wait. VK_WHOLE_SIZE
  // copies the source's remaining bytes. All buffers must share one context.
  // Validate the entire batch before recording; an empty batch is a no-op.
  // Access to the context's queue must be externally synchronized.
  static Status CopyBuffers(std::span<const BatchBufferCopy> copies);

  Status CopyToImage(VulkanImage& dst_image, const uint32_t width,
                     const uint32_t height, const uint32_t layers = 1);

  Status MapData(const std::function<void(void*)>& func);

  VkDeviceSize Size() const { return buffer_size_; }

  VkBuffer buffer = VK_NULL_HANDLE;

  // TODO: keep them public
 private:
  VulkanContext* context_ = nullptr;
  VkDeviceSize buffer_size_ = 0;
  VkMemoryPropertyFlags memory_properties_ = 0;
  VkBufferUsageFlags usage_ = 0;
  VkDeviceMemory buffer_memory_ = VK_NULL_HANDLE;
  bool initialized_ = false;
};

}  // namespace vulkan
}  // namespace core
