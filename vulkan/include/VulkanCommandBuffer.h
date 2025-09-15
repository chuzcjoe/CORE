#pragma once

#include <vector>

#include "VulkanContext.h"
#include "VulkanUtils.h"

namespace core {
namespace vulkan {

class VulkanCommandBuffer {
 public:
  explicit VulkanCommandBuffer(
      VulkanContext* context,
      core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Compute);
  ~VulkanCommandBuffer();

  void Submit(const VkFence fence) const;

  void Reset();

  VkCommandBuffer buffer() const { return command_buffer_; }

 private:
  VulkanContext* context_;
  VkCommandPool command_pool_;
  VkCommandBuffer command_buffer_;
};

}  // namespace vulkan
}  // namespace core