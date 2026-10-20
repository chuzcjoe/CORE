#include <gtest/gtest.h>

#include "ComputeSum.h"
#include "Mat.h"
#include "Status.h"
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

  auto command_buffer_result =
      core::vulkan::VulkanCommandBuffer::Create(&context);
  if (!command_buffer_result.ok()) {
    FAIL() << command_buffer_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanCommandBuffer> command_buffer_owner =
      std::move(*command_buffer_result);
  core::vulkan::VulkanCommandBuffer& command_buffer = *command_buffer_owner;

  auto fence_result = core::vulkan::VulkanFence::Create(&context);
  if (!fence_result.ok()) {
    FAIL() << fence_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanFence> fence_owner =
      std::move(*fence_result);
  core::vulkan::VulkanFence& fence = *fence_owner;

  auto query_pool_result =
      core::vulkan::VulkanQueryPool::Create(&context, VK_QUERY_TYPE_TIMESTAMP);
  if (!query_pool_result.ok()) {
    FAIL() << query_pool_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanQueryPool> query_pool_owner =
      std::move(*query_pool_result);
  core::vulkan::VulkanQueryPool& query_pool = *query_pool_owner;
  core::Mat<int, 1> mat(6000, 6000);
  mat.Fill(3);
  core::Timer t;
  const VkDeviceSize buffer_size = mat.rows() * mat.cols() * sizeof(int);

  auto input_buffer_result = core::vulkan::VulkanBuffer::Create(
      &context, buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!input_buffer_result.ok()) {
    FAIL() << input_buffer_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanBuffer> input_buffer_owner =
      std::move(*input_buffer_result);
  core::vulkan::VulkanBuffer& input_buffer = *input_buffer_owner;

  auto sum_buffer_result = core::vulkan::VulkanBuffer::Create(
      &context, 1 * sizeof(int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!sum_buffer_result.ok()) {
    FAIL() << sum_buffer_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanBuffer> sum_buffer_owner =
      std::move(*sum_buffer_result);
  core::vulkan::VulkanBuffer& sum_buffer = *sum_buffer_owner;

  // Fill buffers
  {
    const core::Status status = input_buffer.MapData([&mat](void* data) {
      memcpy(data, mat.data(), mat.total() * sizeof(int));
    });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = sum_buffer.MapData([](void* data) {
      int zero = 0;
      memcpy(data, &zero, sizeof(int));
    });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  // Create and run compute sum pipeline
  std::unique_ptr<core::vulkan::ComputeSum> compute_sum =
      std::make_unique<core::vulkan::ComputeSum>(&context, input_buffer, sum_buffer, mat.cols(),
                                                 mat.rows());
  {
    const core::Status status = compute_sum->Init();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  {
    const core::Status status = fence.Reset();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  {
    const core::Status status = command_buffer.BeginCommandBufferWithReset();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = query_pool.Reset(command_buffer.buffer());
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = query_pool.Query(
        command_buffer.buffer(), 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = compute_sum->Run(command_buffer.buffer());
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = query_pool.Query(
        command_buffer.buffer(), 1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  VkSubmitInfo submit_info{};
  {
    const core::Status status = command_buffer.Submit(fence.fence, submit_info);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  vkWaitForFences(context.logical_device, 1, &fence.fence, VK_TRUE, UINT64_MAX);

  // Check data
  int result = 0;
  {
    const core::Status status = sum_buffer.MapData(
        [&result](void* data) { memcpy(&result, data, sizeof(int)); });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  printf("GPU Result: %d\n", result);

  {
    const core::Status status = query_pool.GetQueryResults();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
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
