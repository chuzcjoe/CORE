#pragma once

#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanFence {
 public:
  explicit VulkanFence(VulkanContext* context);
  ~VulkanFence();

  void Reset();

  VkFence fence;

 private:
  VulkanContext* context_;
};

}  // namespace vulkan
}  // namespace core