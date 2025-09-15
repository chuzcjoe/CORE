#include <gtest/gtest.h>

#include "ComputeSum.h"
#include "Mat.h"
#include "Timer.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanQueryPool.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

namespace core {
namespace test {

TEST(ComputeSum, test) {
  // Setup Vulkan
  core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Compute;
  core::vulkan::VulkanContext context(true, queue_family_type, nullptr);
  context.Init();
  core::vulkan::VulkanCommandBuffer command_buffer(&context);
  core::vulkan::VulkanFence fence(&context);
  core::vulkan::VulkanQueryPool query_pool(&context, VK_QUERY_TYPE_TIMESTAMP);
  core::Mat<int, 1> mat(6000, 6000);
  mat.Fill(3);
  core::Timer t;
  const VkDeviceSize buffer_size = mat.rows() * mat.cols() * sizeof(int);

  // Create buffers
  core::vulkan::VulkanBuffer input_buffer(
      &context, buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  core::vulkan::VulkanBuffer sum_buffer(
      &context, 1 * sizeof(int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Fill buffers
  input_buffer.MapData([&mat](void* data) { memcpy(data, mat.data(), mat.total() * sizeof(int)); });
  sum_buffer.MapData([](void* data) {
    int zero = 0;
    memcpy(data, &zero, sizeof(int));
  });

  // Create and run compute sum pipeline
  std::unique_ptr<core::vulkan::ComputeSum> compute_sum =
      std::make_unique<core::vulkan::ComputeSum>(&context, input_buffer, sum_buffer, mat.cols(),
                                                 mat.rows());
  compute_sum->Init();

  fence.Reset();

  vkResetCommandBuffer(command_buffer.buffer(), 0);
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  vkBeginCommandBuffer(command_buffer.buffer(), &begin_info);
  query_pool.Reset(command_buffer.buffer());
  query_pool.Query(command_buffer.buffer(), 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  compute_sum->Run(command_buffer.buffer());
  query_pool.Query(command_buffer.buffer(), 1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  VkSubmitInfo submit_info{};
  command_buffer.Submit(fence.fence, submit_info);

  vkWaitForFences(context.logical_device, 1, &fence.fence, VK_TRUE, UINT64_MAX);

  // Check data
  int result = 0;
  sum_buffer.MapData([&result](void* data) { memcpy(&result, data, sizeof(int)); });
  printf("GPU Result: %d\n", result);

  query_pool.GetQueryResults();
  const auto timestamps = query_pool.GetTimeStamps();
  const auto runtime_ms = (timestamps[1] - timestamps[0]) * (context.timestamp_period / 1000000.0);
  printf("GPU time: %fms\n", runtime_ms);

  int cpu_sum = 0;
  t.start();
  for (int row = 0; row < mat.rows(); ++row) {
    for (int col = 0; col < mat.cols(); ++col) {
      cpu_sum += *mat(row, col);
    }
  }
  t.end();
  printf("CPU Result: %d, run time: %fms\n", cpu_sum, t.time());

  EXPECT_EQ(result, mat.rows() * mat.cols() * 3);
}

}  // namespace test
}  // namespace core