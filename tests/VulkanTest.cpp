#include <gtest/gtest.h>

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanUtils.h"

namespace core {
namespace test {

TEST(VulkanTest, test) {
  core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Compute;
  core::vulkan::VulkanContext context(true, queue_family_type);

  core::vulkan::VulkanBuffer buffer(
      &context, 3, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  core::vulkan::VulkanImage image(&context, 128, 128, VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  EXPECT_TRUE(context.instance != VK_NULL_HANDLE);
}
}  // namespace test
}  // namespace core