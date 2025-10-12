#pragma once

#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanFence {
 public:
  explicit VulkanFence(VulkanContext* context);
  ~VulkanFence();

  void Reset();

  VkFence fence = VK_NULL_HANDLE;

 private:
  VulkanContext* context_ = nullptr;
};

class VulkanSemaphore {
 public:
  explicit VulkanSemaphore(VulkanContext* context);
  ~VulkanSemaphore();

  VkSemaphore semaphore = VK_NULL_HANDLE;

 private:
  VulkanContext* context_ = nullptr;
};

}  // namespace vulkan
}  // namespace core