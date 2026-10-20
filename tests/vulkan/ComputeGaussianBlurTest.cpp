#include <gtest/gtest.h>

#include <vector>

#include "ComputeGaussianBlur.h"
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

TEST(ComputeGaussianBlur, test) {
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
  core::Mat<float, 1> mat(3000, 4000);
  mat.Fill(1);
  core::Timer timer;
  const VkDeviceSize buffer_size = mat.rows() * mat.cols() * sizeof(float);

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

  auto dst_buffer_result = core::vulkan::VulkanBuffer::Create(
      &context, buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!dst_buffer_result.ok()) {
    FAIL() << dst_buffer_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanBuffer> dst_buffer_owner =
      std::move(*dst_buffer_result);
  core::vulkan::VulkanBuffer& dst_buffer = *dst_buffer_owner;

  // Fill buffers
  {
    const core::Status status = input_buffer.MapData([&mat](void* data) {
      memcpy(data, mat.data(), mat.total() * sizeof(float));
    });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  // Create and run compute sum pipeline
  timer.start();
  std::unique_ptr<core::vulkan::ComputeGaussianBlur> compute_blur =
      std::make_unique<core::vulkan::ComputeGaussianBlur>(&context, input_buffer, dst_buffer,
                                                          mat.cols(), mat.rows());
  {
    const core::Status status = compute_blur->Init();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  timer.end();
  printf("Create compute blur pipeline: %fms\n", timer.time());

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
    const core::Status status = compute_blur->Run(command_buffer.buffer());
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

  {
    const core::Status status = query_pool.GetQueryResults();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  const auto timestamps = query_pool.GetTimeStamps();
  const auto runtime_ms = (timestamps[1] - timestamps[0]) * (context.timestamp_period / 1000000.0);
  printf("GPU time: %fms\n", runtime_ms);

  core::Mat<float, 1> mat_blur(3000, 4000);
  std::vector<float> gaussian_kernel = {0.0625f, 0.125f,  0.0625f, 0.125f, 0.25f,
                                        0.125f,  0.0625f, 0.125f,  0.0625f};
  timer.start();
  for (int row = 0; row < mat.rows(); ++row) {
    for (int col = 0; col < mat.cols(); ++col) {
      if (row == 0 || row == (mat.rows() - 1) || col == 0 || col == (mat.cols() - 1)) {
        *mat_blur(row, col) = *mat(row, col);
        continue;
      }
      float sum = 0.0f;
      int idx = 0;
      for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
          sum += (*mat(row + i, col + j)) * gaussian_kernel[idx];
          ++idx;
        }
      }
      *mat_blur(row, col) = sum;
    }
  }
  timer.end();
  printf("CPU time: %fms\n", timer.time());

  // check data
  core::Mat<float, 1> blur_cpu(3000, 4000);
  {
    const core::Status status = dst_buffer.MapData([&blur_cpu](void* data) {
      memcpy(blur_cpu.data(), data, sizeof(float) * blur_cpu.total());
    });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  for (int row = 0; row < blur_cpu.rows(); ++row) {
    for (int col = 0; col < blur_cpu.cols(); ++col) {
      EXPECT_EQ(*blur_cpu(row, col), *mat_blur(row, col));
    }
  }
}

}  // namespace test
}  // namespace core
