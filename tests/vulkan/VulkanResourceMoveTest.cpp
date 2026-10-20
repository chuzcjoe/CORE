#include <gtest/gtest.h>

#include <cstdint>
#include <utility>

#include "Status.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"

namespace core {
namespace test {

TEST(VulkanResourceMoveTest, CommandBufferMovesPreserveInitializationState) {
  auto context_result = vulkan::VulkanContext::Create();
  ASSERT_TRUE(context_result.ok()) << context_result.status().message();
  auto context = std::move(*context_result);
  ASSERT_TRUE(context->Init().ok());
  auto commands = vulkan::VulkanCommandBuffer::Create(context.get());
  ASSERT_TRUE(commands.ok()) << commands.status().message();
  const VkCommandBuffer handle = (*commands)->buffer();
  vulkan::VulkanCommandBuffer moved(std::move(**commands));
  EXPECT_EQ(moved.buffer(), handle);
  EXPECT_EQ(moved.Init(context.get()).code(), ErrorCode::kInvalidState);
  vulkan::VulkanCommandBuffer assigned;
  assigned = std::move(moved);
  EXPECT_EQ(assigned.buffer(), handle);
  EXPECT_EQ(assigned.Init(context.get()).code(), ErrorCode::kInvalidState);
  EXPECT_TRUE(assigned.Reset().ok());
}

TEST(VulkanResourceMoveTest, BufferSelfMovePreservesHandleAndContents) {
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

  auto buffer_result = vulkan::VulkanBuffer::Create(
      &context, sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!buffer_result.ok()) {
    FAIL() << buffer_result.status().message();
  }
  std::unique_ptr<vulkan::VulkanBuffer> buffer_owner =
      std::move(*buffer_result);
  vulkan::VulkanBuffer& buffer = *buffer_owner;
  {
    const core::Status status =
        buffer.MapData([](void* data) { *static_cast<uint32_t*>(data) = 42; });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  const VkBuffer original_buffer = buffer.buffer;
  vulkan::VulkanBuffer* alias = &buffer;

  buffer = std::move(*alias);

  ASSERT_EQ(buffer.buffer, original_buffer);
  EXPECT_EQ(buffer.Size(), sizeof(uint32_t));
  {
    const core::Status status = buffer.MapData(
        [](void* data) { EXPECT_EQ(*static_cast<uint32_t*>(data), 42U); });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  vulkan::VulkanBuffer destination;
  destination = std::move(buffer);
  EXPECT_EQ(buffer.buffer, VK_NULL_HANDLE);
  EXPECT_EQ(destination.buffer, original_buffer);
  {
    const core::Status status = destination.MapData(
        [](void* data) { EXPECT_EQ(*static_cast<uint32_t*>(data), 42U); });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
}

TEST(VulkanResourceMoveTest, ImageSelfMovePreservesOwnedHandles) {
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

  auto image_result = vulkan::VulkanImage::Create(
      &context, 8, 4, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (!image_result.ok()) {
    FAIL() << image_result.status().message();
  }
  std::unique_ptr<vulkan::VulkanImage> image_owner = std::move(*image_result);
  vulkan::VulkanImage& image = *image_owner;
  const VkImage original_image = image.image;
  const VkImageView original_view = image.image_view;
  const VkDeviceMemory original_memory = image.image_memory;
  vulkan::VulkanImage* alias = &image;

  image = std::move(*alias);

  ASSERT_EQ(image.image, original_image);
  EXPECT_EQ(image.image_view, original_view);
  EXPECT_EQ(image.image_memory, original_memory);
  EXPECT_EQ(image.image_width, 8U);
  EXPECT_EQ(image.image_height, 4U);

  vulkan::VulkanImage destination;
  destination = std::move(image);
  EXPECT_EQ(image.image, VK_NULL_HANDLE);
  EXPECT_EQ(image.image_view, VK_NULL_HANDLE);
  EXPECT_EQ(image.image_memory, VK_NULL_HANDLE);
  EXPECT_EQ(destination.image, original_image);
  EXPECT_EQ(destination.image_view, original_view);
  EXPECT_EQ(destination.image_memory, original_memory);
  EXPECT_EQ(destination.image_width, 8U);
  EXPECT_EQ(destination.image_height, 4U);
}

}  // namespace test
}  // namespace core
