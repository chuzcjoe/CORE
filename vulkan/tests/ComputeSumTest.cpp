#include <gtest/gtest.h>

#include "ComputeSum.h"
#include "Mat.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanUtils.h"

namespace core {
namespace test {

TEST(ComputeSum, test) {
  core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Compute;
  core::vulkan::VulkanContext context(true, queue_family_type);

  core::Mat<float, 1> mat(1000, 1000);
  mat.Fill(3.0f);

  const VkDeviceSize buffer_size = mat.rows() * mat.cols() * sizeof(float);

  core::vulkan::VulkanBuffer input_buffer(
      &context, buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  core::vulkan::VulkanBuffer sum_buffer(
      &context, 1 * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  input_buffer.MapData(
      [&mat](void* data) { memcpy(data, mat.data(), mat.total() * sizeof(float)); });
  sum_buffer.MapData([](void* data) {
    float zero = 0.0f;
    memcpy(data, &zero, sizeof(float));
  });

  // VkFence fence;
  // core::vulkan::ComputeSum compute_sum(&context, input_buffer, mat.cols(), mat.rows());
  // compute_sum.Init();
  // compute_sum.Run();
}

}  // namespace test
}  // namespace core