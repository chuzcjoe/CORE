#include <gtest/gtest.h>

#include "VulkanContext.h"

namespace core {

TEST(VulkanTest, test) {
  core::vulkan::VulkanContext context(true);
  EXPECT_TRUE(context.instance != VK_NULL_HANDLE);
}
}  // namespace core