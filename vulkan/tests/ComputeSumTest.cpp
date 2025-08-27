#include <gtest/gtest.h>

#include "ComputeSum.h"
#include "Mat.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

namespace core {
namespace test {

TEST(ComputeSum, test) {
  core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Compute;
  core::vulkan::VulkanContext context(true, queue_family_type);
  core::vulkan::VulkanCommandBuffer command_buffer(&context);
  core::vulkan::VulkanFence fence(&context);

  core::Mat<int, 1> mat(1000, 1000);
  mat.Fill(3);

  const VkDeviceSize buffer_size = mat.rows() * mat.cols() * sizeof(int);

  core::vulkan::VulkanBuffer input_buffer(
      &context, buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  core::vulkan::VulkanBuffer sum_buffer(
      &context, 1 * sizeof(int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  input_buffer.MapData([&mat](void* data) { memcpy(data, mat.data(), mat.total() * sizeof(int)); });
  sum_buffer.MapData([](void* data) {
    int zero = 0;
    memcpy(data, &zero, sizeof(int));
  });

  std::unique_ptr<core::vulkan::ComputeSum> compute_sum =
      std::make_unique<core::vulkan::ComputeSum>(&context, input_buffer, sum_buffer, mat.cols(),
                                                 mat.rows());
  compute_sum->Init();

  fence.Reset();
  vkResetCommandBuffer(command_buffer.buffer(), 0);
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(command_buffer.buffer(), &begin_info);

  compute_sum->Run(command_buffer.buffer());

  command_buffer.Submit(fence.fence);
  vkWaitForFences(context.logical_device, 1, &fence.fence, VK_TRUE, UINT64_MAX);

  // Check data
  int result = 0;
  sum_buffer.MapData([&result](void* data) { memcpy(&result, data, sizeof(int)); });
  printf("GPU Result: %d\n", result);
  printf("CPU Result: %d\n", mat.rows() * mat.cols() * 3);
  EXPECT_EQ(result, mat.rows() * mat.cols() * 3);
}

}  // namespace test
}  // namespace core