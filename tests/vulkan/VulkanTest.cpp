#include <gtest/gtest.h>

#include "Status.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanQueryPool.h"
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

  auto context_result =
      core::vulkan::VulkanContext::Create(true, queue_family_type, nullptr);
  if (!context_result.ok()) {
    FAIL() << context_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanContext> context_owner =
      std::move(*context_result);
  core::vulkan::VulkanContext& context = *context_owner;
  {
    const core::Status status = context.Init();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  const VkDeviceSize buffer_size = 3 * sizeof(float);

  auto buffer_result = core::vulkan::VulkanBuffer::Create(
      &context, buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!buffer_result.ok()) {
    FAIL() << buffer_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanBuffer> buffer_owner =
      std::move(*buffer_result);
  core::vulkan::VulkanBuffer& buffer = *buffer_owner;

  auto uniform_buffer_result = core::vulkan::VulkanBuffer::Create(
      &context, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!uniform_buffer_result.ok()) {
    FAIL() << uniform_buffer_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanBuffer> uniform_buffer_owner =
      std::move(*uniform_buffer_result);

  auto image_result = core::vulkan::VulkanImage::Create(
      &context, 128, 128, VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (!image_result.ok()) {
    FAIL() << image_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanImage> image_owner =
      std::move(*image_result);

  {
    const core::Status status = buffer.MapData([](void* data) {
      float arr[3] = {1.0f, 2.0f, 3.0f};
      memcpy(data, arr, 3 * sizeof(float));
    });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  EXPECT_TRUE(context.instance != VK_NULL_HANDLE);
}

TEST(VulkanQueryPoolTest, RejectsUnsupportedQueryTypes) {
  auto context_result = core::vulkan::VulkanContext::Create(
      true, core::vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
  if (!context_result.ok()) {
    FAIL() << context_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanContext> context_owner =
      std::move(*context_result);
  core::vulkan::VulkanContext& context = *context_owner;
  {
    const core::Status status = context.Init();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  const VkQueryType unsupported_types[] = {VK_QUERY_TYPE_OCCLUSION,
                                          VK_QUERY_TYPE_PIPELINE_STATISTICS};
  for (const VkQueryType query_type : unsupported_types) {
    const auto result =
        core::vulkan::VulkanQueryPool::Create(&context, query_type);
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.status().code(), core::ErrorCode::kUnsupported);
    EXPECT_EQ(result.status().message(), "Query type is not supported");
  }
}

}  // namespace test
}  // namespace core
