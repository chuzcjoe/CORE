#include <gtest/gtest.h>

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"

namespace core {
namespace test {

TEST(VulkanTest, test) {
  core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Compute;
  core::vulkan::VulkanContext context(true, queue_family_type);
  core::vulkan::VulkanBuffer buffer(
      &context, 3, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  EXPECT_TRUE(context.instance != VK_NULL_HANDLE);
}
}  // namespace test
}  // namespace core