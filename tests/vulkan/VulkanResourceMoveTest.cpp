#include <gtest/gtest.h>

#include <cstdint>
#include <utility>

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"

namespace core {
namespace test {

TEST(VulkanResourceMoveTest, BufferSelfMovePreservesHandleAndContents) {
  vulkan::VulkanContext context(true, vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
  context.Init();
  vulkan::VulkanBuffer buffer(
      &context, sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  buffer.MapData([](void* data) { *static_cast<uint32_t*>(data) = 42; });
  const VkBuffer original_buffer = buffer.buffer;
  vulkan::VulkanBuffer* alias = &buffer;

  buffer = std::move(*alias);

  ASSERT_EQ(buffer.buffer, original_buffer);
  EXPECT_EQ(buffer.Size(), sizeof(uint32_t));
  buffer.MapData([](void* data) { EXPECT_EQ(*static_cast<uint32_t*>(data), 42U); });

  vulkan::VulkanBuffer destination;
  destination = std::move(buffer);
  EXPECT_EQ(buffer.buffer, VK_NULL_HANDLE);
  EXPECT_EQ(destination.buffer, original_buffer);
  destination.MapData([](void* data) { EXPECT_EQ(*static_cast<uint32_t*>(data), 42U); });
}

TEST(VulkanResourceMoveTest, ImageSelfMovePreservesOwnedHandles) {
  vulkan::VulkanContext context(true, vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
  context.Init();
  vulkan::VulkanImage image(&context, 8, 4, VK_FORMAT_R8G8B8A8_UNORM,
                            VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
