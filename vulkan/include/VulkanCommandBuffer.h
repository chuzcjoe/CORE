#pragma once

#include <vector>

#include "VulkanContext.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

namespace core {
namespace vulkan {

class VulkanCommandBuffer {
 public:
  explicit VulkanCommandBuffer(VulkanContext* context);
  ~VulkanCommandBuffer();

  void Submit(const VkFence& fence, VkSubmitInfo& submit_info) const;
  void Submit(const VkFence& fence) const;

  void Reset();
  static VulkanCommandBuffer BeginOneTimeCommands(VulkanContext* context);
  void EndOneTimeCommands() const;

  VkCommandBuffer buffer() const { return command_buffer_; }

 private:
  VulkanContext* context_ = nullptr;
  QueueFamilyType queue_family_type_;
  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
};

}  // namespace vulkan
}  // namespace core