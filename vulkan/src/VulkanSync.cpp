#include "VulkanSync.h"

namespace core {
namespace vulkan {

VulkanFence::VulkanFence(VulkanContext* context) : context_(context) {
  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled so first wait is not blocking

  VK_CHECK(vkCreateFence(context_->logical_device, &fence_info, nullptr, &fence));
}

VulkanFence::~VulkanFence() { vkDestroyFence(context_->logical_device, fence, nullptr); }

void VulkanFence::Reset() { VK_CHECK(vkResetFences(context_->logical_device, 1, &fence)); }

VulkanSemaphore::VulkanSemaphore(VulkanContext* context) : context_(context) {
  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VK_CHECK(vkCreateSemaphore(context_->logical_device, &semaphore_info, nullptr, &semaphore));
}

VulkanSemaphore::~VulkanSemaphore() {
  vkDestroySemaphore(context_->logical_device, semaphore, nullptr);
}

}  // namespace vulkan
}  // namespace core