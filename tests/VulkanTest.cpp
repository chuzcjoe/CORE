#include <gtest/gtest.h>

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanUtils.h"

namespace core {
namespace test {

struct UniformBufferObject {
  int width;
  int height;
  float scale;
};

TEST(VulkanTest, test) {
  core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Compute;
  core::vulkan::VulkanContext context(true, nullptr, queue_family_type);

  const VkDeviceSize buffer_size = 3 * sizeof(float);
  core::vulkan::VulkanBuffer buffer(
      &context, buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  core::vulkan::VulkanBuffer uniform_buffer(
      &context, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  core::vulkan::VulkanImage image(&context, 128, 128, VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  buffer.MapData([](void* data) {
    float arr[3] = {1.0f, 2.0f, 3.0f};
    memcpy(data, arr, 3 * sizeof(float));
  });

  EXPECT_TRUE(context.instance != VK_NULL_HANDLE);
}
}  // namespace test
}  // namespace core