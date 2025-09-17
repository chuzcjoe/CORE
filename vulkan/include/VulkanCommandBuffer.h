#pragma once

#include <vector>

#include "VulkanContext.h"
#include "VulkanUtils.h"

namespace core {
namespace vulkan {

class VulkanCommandBuffer {
 public:
  explicit VulkanCommandBuffer(VulkanContext* context);
  ~VulkanCommandBuffer();

  void Submit(const VkFence fence, VkSubmitInfo& submit_info) const;

  void Reset();

  VkCommandBuffer buffer() const { return command_buffer_; }

 private:
  VulkanContext* context_;
  QueueFamilyType queue_family_type_;
  VkCommandPool command_pool_;
  VkCommandBuffer command_buffer_;
};

}  // namespace vulkan
}  // namespace core