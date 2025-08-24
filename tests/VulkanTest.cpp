#include <gtest/gtest.h>

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"

namespace core {
namespace test {

TEST(VulkanTest, test) {
  core::vulkan::VulkanContext context(true, core::vulkan::QueueFamilyType::Compute);
  EXPECT_TRUE(context.instance != VK_NULL_HANDLE);

  core::vulkan::VulkanBuffer buffer(
      &context, 3, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}
}  // namespace test
}  // namespace core