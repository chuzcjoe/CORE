#include <gtest/gtest.h>

#include <stdexcept>

#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanSync.h"

namespace core {
namespace test {

TEST(VulkanCommandBufferTest, RejectsExtraWaitStageMasks) {
  vulkan::VulkanContext context(true, vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
  context.Init();
  vulkan::VulkanCommandBuffer command_buffer(&context);
  command_buffer.BeginCommandBuffer();
  const vulkan::SubmitSyncInfo sync_info{
      .wait_semaphores = {},
      .wait_stage_masks = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
      .signal_semaphores = {},
  };

  EXPECT_THROW(command_buffer.Submit(VK_NULL_HANDLE, sync_info), std::invalid_argument);
  VK_CHECK(vkQueueWaitIdle(context.compute_queue()));
}

TEST(VulkanCommandBufferTest, RejectsMissingWaitStageMasksWithoutEndingRecording) {
  vulkan::VulkanContext context(true, vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
  context.Init();
  vulkan::VulkanCommandBuffer command_buffer(&context);
  vulkan::VulkanSemaphore semaphore(&context);
  vulkan::VulkanFence fence(&context);
  fence.Reset();
  command_buffer.BeginCommandBuffer();
  const vulkan::SubmitSyncInfo invalid_sync_info{
      .wait_semaphores = {semaphore.semaphore},
      .wait_stage_masks = {},
      .signal_semaphores = {},
  };

  ASSERT_THROW(command_buffer.Submit(fence.fence, invalid_sync_info), std::invalid_argument);
  const vulkan::SubmitSyncInfo valid_sync_info{};
  command_buffer.Submit(fence.fence, valid_sync_info);
  VK_CHECK(vkWaitForFences(context.logical_device, 1, &fence.fence, VK_TRUE, UINT64_MAX));
}

}  // namespace test
}  // namespace core
