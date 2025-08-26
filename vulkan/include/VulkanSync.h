#pragma once

#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanFence {
 public:
  explicit VulkanFence(VulkanContext* context);
  ~VulkanFence();

  void Reset();
  VkFence Fence() const { return fence_; }

 private:
  VulkanContext* context_;
  VkFence fence_;
};

}  // namespace vulkan
}  // namespace core