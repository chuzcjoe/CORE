#include <gtest/gtest.h>

#include "VulkanContext.h"
#include "VulkanUtils.h"

namespace core {
namespace test {

TEST(VulkanTest, test) {
  core::vulkan::VulkanContext context(true, core::vulkan::QueueFamilyType::Compute);
  EXPECT_TRUE(context.instance != VK_NULL_HANDLE);
}
}  // namespace test
}  // namespace core