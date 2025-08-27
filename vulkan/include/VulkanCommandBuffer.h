#pragma once

#include <vector>

#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanCommandBuffer {
 public:
  explicit VulkanCommandBuffer(VulkanContext* context);
  ~VulkanCommandBuffer();

  void Submit(const VkFence fence) const;

  VkCommandBuffer buffer() const { return command_buffer_; }

 private:
  VulkanContext* context_;
  VkCommandPool command_pool_;
  VkCommandBuffer command_buffer_;
};

}  // namespace vulkan
}  // namespace core