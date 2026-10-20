#include "VulkanSync.h"

namespace core {
namespace vulkan {

Result<std::unique_ptr<VulkanFence>> VulkanFence::Create(
    VulkanContext* context) {
  std::unique_ptr<VulkanFence> object(new VulkanFence());
  const Status status = object->Init(context);
  if (!status.ok()) {
    return Result<std::unique_ptr<VulkanFence>>(status);
  }
  return Result<std::unique_ptr<VulkanFence>>(std::move(object));
}

Status VulkanFence::Init(VulkanContext* context) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanFence initialization is single-use");
  }
  initialized_ = true;
  context_ = context;
  if (context_ == nullptr || !context_->is_initialized()) {
    return Status(ErrorCode::kInvalidState, "A live Vulkan device is required");
  }

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled so first wait is not blocking

  {
    const VkResult result =
        vkCreateFence(context_->logical_device, &fence_info, nullptr, &fence);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

VulkanFence::~VulkanFence() {
  if (context_ == nullptr || context_->logical_device == VK_NULL_HANDLE) {
    return;
  }
  vkDestroyFence(context_->logical_device, fence, nullptr);
}

Status VulkanFence::Reset() {
  if (context_ == nullptr || fence == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanFence requires successful initialization");
  }
  {
    const VkResult result = vkResetFences(context_->logical_device, 1, &fence);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }
  return Status();
}

Result<std::unique_ptr<VulkanSemaphore>> VulkanSemaphore::Create(
    VulkanContext* context) {
  std::unique_ptr<VulkanSemaphore> object(new VulkanSemaphore());
  const Status status = object->Init(context);
  if (!status.ok()) {
    return Result<std::unique_ptr<VulkanSemaphore>>(status);
  }
  return Result<std::unique_ptr<VulkanSemaphore>>(std::move(object));
}

Status VulkanSemaphore::Init(VulkanContext* context) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanSemaphore initialization is single-use");
  }
  initialized_ = true;
  context_ = context;
  if (context_ == nullptr || !context_->is_initialized()) {
    return Status(ErrorCode::kInvalidState, "A live Vulkan device is required");
  }

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  {
    const VkResult result = vkCreateSemaphore(
        context_->logical_device, &semaphore_info, nullptr, &semaphore);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

VulkanSemaphore::~VulkanSemaphore() {
  if (context_ == nullptr || context_->logical_device == VK_NULL_HANDLE) {
    return;
  }

  vkDestroySemaphore(context_->logical_device, semaphore, nullptr);
}

}  // namespace vulkan
}  // namespace core
