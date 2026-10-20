#pragma once

#include <span>
#include <vector>

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanSync.h"

namespace core::vulkan {

// Synchronous, reusable buffer transfers on one context's queue. The initialized
// context must outlive this object. Externally synchronize this object and queue.
class VulkanBufferTransfer {
 public:
  explicit VulkanBufferTransfer(VulkanContext& context);
  VulkanBufferTransfer(const VulkanBufferTransfer&) = delete;
  VulkanBufferTransfer& operator=(const VulkanBufferTransfer&) = delete;
  VulkanBufferTransfer(VulkanBufferTransfer&&) = delete;
  VulkanBufferTransfer& operator=(VulkanBufferTransfer&&) = delete;

  // Same range validation, copy order and visibility as VulkanBuffer::CopyBuffers.
  // Returns after completion; buffers and input descriptors need only survive
  // the call. Invalid batches execute nothing and leave the session reusable.
  void CopyBuffers(std::span<const BatchBufferCopy> copies);

 private:
  VulkanContext& context_;
  VulkanCommandBuffer command_buffer_;
  VulkanFence fence_;
  std::vector<VkBufferCopy> regions_;
};

}  // namespace core::vulkan
