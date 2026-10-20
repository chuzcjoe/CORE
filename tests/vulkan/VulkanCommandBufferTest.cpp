#include <gtest/gtest.h>

#include "Status.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanSync.h"

namespace core {
namespace test {

TEST(VulkanCommandBufferTest, RejectsExtraWaitStageMasks) {
  auto context_result = vulkan::VulkanContext::Create(
      true, vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
  if (!context_result.ok()) {
    FAIL() << context_result.status().message();
  }
  std::unique_ptr<vulkan::VulkanContext> context_owner =
      std::move(*context_result);
  vulkan::VulkanContext& context = *context_owner;
  {
    const core::Status status = context.Init();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  auto command_buffer_result = vulkan::VulkanCommandBuffer::Create(&context);
  if (!command_buffer_result.ok()) {
    FAIL() << command_buffer_result.status().message();
  }
  std::unique_ptr<vulkan::VulkanCommandBuffer> command_buffer_owner =
      std::move(*command_buffer_result);
  vulkan::VulkanCommandBuffer& command_buffer = *command_buffer_owner;
  {
    const core::Status status = command_buffer.BeginCommandBuffer();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  const vulkan::SubmitSyncInfo sync_info{
      .wait_semaphores = {},
      .wait_stage_masks = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
      .signal_semaphores = {},
  };

  EXPECT_EQ((command_buffer.Submit(VK_NULL_HANDLE, sync_info)).code(),
            core::ErrorCode::kInvalidArgument);
  {
    const VkResult error = vkQueueWaitIdle(context.compute_queue());
    if (error != VK_SUCCESS) {
      FAIL() << core::Status(core::ErrorCode::kBackend,
                             core::vulkan::VkErrorMessages(error), "Vulkan",
                             error)
                    .message();
    }
  }
}

TEST(VulkanCommandBufferTest, RejectsMissingWaitStageMasksWithoutEndingRecording) {
  auto context_result = vulkan::VulkanContext::Create(
      true, vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
  if (!context_result.ok()) {
    FAIL() << context_result.status().message();
  }
  std::unique_ptr<vulkan::VulkanContext> context_owner =
      std::move(*context_result);
  vulkan::VulkanContext& context = *context_owner;
  {
    const core::Status status = context.Init();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  auto command_buffer_result = vulkan::VulkanCommandBuffer::Create(&context);
  if (!command_buffer_result.ok()) {
    FAIL() << command_buffer_result.status().message();
  }
  std::unique_ptr<vulkan::VulkanCommandBuffer> command_buffer_owner =
      std::move(*command_buffer_result);
  vulkan::VulkanCommandBuffer& command_buffer = *command_buffer_owner;

  auto semaphore_result = vulkan::VulkanSemaphore::Create(&context);
  if (!semaphore_result.ok()) {
    FAIL() << semaphore_result.status().message();
  }
  std::unique_ptr<vulkan::VulkanSemaphore> semaphore_owner =
      std::move(*semaphore_result);
  vulkan::VulkanSemaphore& semaphore = *semaphore_owner;

  auto fence_result = vulkan::VulkanFence::Create(&context);
  if (!fence_result.ok()) {
    FAIL() << fence_result.status().message();
  }
  std::unique_ptr<vulkan::VulkanFence> fence_owner = std::move(*fence_result);
  vulkan::VulkanFence& fence = *fence_owner;
  {
    const core::Status status = fence.Reset();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = command_buffer.BeginCommandBuffer();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  const vulkan::SubmitSyncInfo invalid_sync_info{
      .wait_semaphores = {semaphore.semaphore},
      .wait_stage_masks = {},
      .signal_semaphores = {},
  };

  ASSERT_EQ((command_buffer.Submit(fence.fence, invalid_sync_info)).code(),
            core::ErrorCode::kInvalidArgument);
  const vulkan::SubmitSyncInfo valid_sync_info{};
  {
    const core::Status status =
        command_buffer.Submit(fence.fence, valid_sync_info);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const VkResult error = vkWaitForFences(context.logical_device, 1,
                                           &fence.fence, VK_TRUE, UINT64_MAX);
    if (error != VK_SUCCESS) {
      FAIL() << core::Status(core::ErrorCode::kBackend,
                             core::vulkan::VkErrorMessages(error), "Vulkan",
                             error)
                    .message();
    }
  }
}

}  // namespace test
}  // namespace core
