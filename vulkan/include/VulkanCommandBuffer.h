#pragma once

#include <memory>
#include <vector>

#include "Status.h"
#include "VulkanContext.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

namespace core {
namespace vulkan {

struct SubmitSyncInfo {
  std::vector<VkSemaphore> wait_semaphores;
  std::vector<VkPipelineStageFlags> wait_stage_masks;
  std::vector<VkSemaphore> signal_semaphores;
};

class VulkanCommandBuffer {
 public:
  VulkanCommandBuffer() = default;

  static Result<std::unique_ptr<VulkanCommandBuffer>> Create(
      VulkanContext* context);
  Status Init(VulkanContext* context);
  VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
  VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;
  VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept;
  VulkanCommandBuffer& operator=(VulkanCommandBuffer&& other) noexcept;
  ~VulkanCommandBuffer();

  Status Submit(const VkFence& fence, VkSubmitInfo& submit_info) const;
  Status Submit(const VkFence& fence, VkSubmitInfo&& submit_info) const;
  Status Submit(const VkFence& fence, const SubmitSyncInfo& sync_info) const;
  Status Submit(const VkFence& fence) const;

  Status Reset();
  Status BeginCommandBuffer();
  Status BeginCommandBufferWithReset();
  static Result<std::unique_ptr<VulkanCommandBuffer>> BeginOneTimeCommands(
      VulkanContext* context);
  Status EndOneTimeCommands() const;

  VkCommandBuffer buffer() const { return command_buffer_; }

 private:
  VulkanContext* context_ = nullptr;
  QueueFamilyType queue_family_type_ = QueueFamilyType::Compute;
  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
  bool initialized_ = false;
};

}  // namespace vulkan
}  // namespace core
