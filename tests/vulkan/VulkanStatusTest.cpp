#include <gtest/gtest.h>

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDynamicRendering.h"
#include "VulkanImage.h"
#include "VulkanQueryPool.h"
#include "VulkanRenderPass.h"
#include "VulkanSampler.h"
#include "VulkanSwapChain.h"
#include "VulkanSync.h"

namespace core::test {
namespace {

TEST(VulkanStatusTest, InvalidParentsAreSafelyDestructible) {
  EXPECT_FALSE(vulkan::VulkanBuffer::Create(nullptr, 16, 0, 0).ok());
  EXPECT_FALSE(vulkan::VulkanImage::Create(nullptr, 1, 1,
                                           VK_FORMAT_R8G8B8A8_UNORM, 0, 0, 0)
                   .ok());
  EXPECT_FALSE(vulkan::VulkanCommandBuffer::Create(nullptr).ok());
  EXPECT_FALSE(vulkan::VulkanDynamicRendering::Create(nullptr).ok());
  EXPECT_FALSE(
      vulkan::VulkanQueryPool::Create(nullptr, VK_QUERY_TYPE_TIMESTAMP).ok());
  EXPECT_FALSE(vulkan::VulkanRenderPass::Create(nullptr).ok());
  EXPECT_FALSE(vulkan::VulkanSampler::Create(nullptr).ok());
  EXPECT_FALSE(vulkan::VulkanSwapChain::Create(nullptr, VK_NULL_HANDLE).ok());
  EXPECT_FALSE(vulkan::VulkanFence::Create(nullptr).ok());
  EXPECT_FALSE(vulkan::VulkanSemaphore::Create(nullptr).ok());
}

TEST(VulkanStatusTest, InertResourcesRejectOperations) {
  vulkan::VulkanBuffer buffer;
  EXPECT_EQ(buffer.CopyToBuffer(buffer).code(), ErrorCode::kInvalidState);
  vulkan::VulkanCommandBuffer commands;
  EXPECT_EQ(commands.BeginCommandBuffer().code(), ErrorCode::kInvalidState);
  vulkan::VulkanFence fence;
  EXPECT_EQ(fence.Reset().code(), ErrorCode::kInvalidState);
}

TEST(VulkanStatusTest, FailedInitializationCannotBeRetried) {
  vulkan::VulkanBuffer buffer;
  EXPECT_FALSE(buffer.Init(nullptr, 16, 0, 0).ok());
  EXPECT_EQ(buffer.Init(nullptr, 16, 0, 0).code(), ErrorCode::kInvalidState);
}

}  // namespace
}  // namespace core::test
