#include <gtest/gtest.h>

#include "VulkanEnv.h"

namespace core {

TEST(VulkanEnvTest, test) {
  core::vulkan::VulkanEnv env(true);
  EXPECT_TRUE(env.instance != VK_NULL_HANDLE);
}
}  // namespace core