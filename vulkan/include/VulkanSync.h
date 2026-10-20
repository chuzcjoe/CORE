#pragma once

#include <memory>

#include "Status.h"
#include "VulkanContext.h"

namespace core::vulkan {

class VulkanFence {
 public:
  VulkanFence() = default;
  VulkanFence(const VulkanFence&) = delete;
  VulkanFence& operator=(const VulkanFence&) = delete;
  static Result<std::unique_ptr<VulkanFence>> Create(VulkanContext* context);
  Status Init(VulkanContext* context);
  ~VulkanFence();
  Status Reset();
  VkFence fence = VK_NULL_HANDLE;

 private:
  VulkanContext* context_ = nullptr;
  bool initialized_ = false;
};

class VulkanSemaphore {
 public:
  VulkanSemaphore() = default;
  VulkanSemaphore(const VulkanSemaphore&) = delete;
  VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;
  static Result<std::unique_ptr<VulkanSemaphore>> Create(
      VulkanContext* context);
  Status Init(VulkanContext* context);
  ~VulkanSemaphore();
  VkSemaphore semaphore = VK_NULL_HANDLE;

 private:
  VulkanContext* context_ = nullptr;
  bool initialized_ = false;
};

}  // namespace core::vulkan
