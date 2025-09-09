#include <gtest/gtest.h>

#include <vector>

#include "ComputeGaussianBlur.h"
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

TEST(ComputeGaussianBlur, test) {
  // Setup Vulkan
  core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Compute;
  core::vulkan::VulkanContext context(true, nullptr, queue_family_type);
  core::vulkan::VulkanCommandBuffer command_buffer(&context);
  core::vulkan::VulkanFence fence(&context);
  core::vulkan::VulkanQueryPool query_pool(&context, VK_QUERY_TYPE_TIMESTAMP);
  core::Mat<float, 1> mat(3000, 4000);
  mat.Fill(1);
  core::Timer t;
  const VkDeviceSize buffer_size = mat.rows() * mat.cols() * sizeof(float);

  // Create buffers
  core::vulkan::VulkanBuffer input_buffer(
      &context, buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  core::vulkan::VulkanBuffer dst_buffer(
      &context, buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Fill buffers
  input_buffer.MapData(
      [&mat](void* data) { memcpy(data, mat.data(), mat.total() * sizeof(float)); });

  // Create and run compute sum pipeline
  std::unique_ptr<core::vulkan::ComputeGaussianBlur> compute_blur =
      std::make_unique<core::vulkan::ComputeGaussianBlur>(&context, input_buffer, dst_buffer,
                                                          mat.cols(), mat.rows());
  compute_blur->Init();

  fence.Reset();

  vkResetCommandBuffer(command_buffer.buffer(), 0);
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  vkBeginCommandBuffer(command_buffer.buffer(), &begin_info);
  query_pool.Reset(command_buffer.buffer());
  query_pool.Query(command_buffer.buffer(), 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  compute_blur->Run(command_buffer.buffer());
  query_pool.Query(command_buffer.buffer(), 1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  command_buffer.Submit(fence.fence);

  vkWaitForFences(context.logical_device, 1, &fence.fence, VK_TRUE, UINT64_MAX);

  query_pool.GetQueryResults();
  const auto timestamps = query_pool.GetTimeStamps();
  const auto runtime_ms = (timestamps[1] - timestamps[0]) * (context.timestamp_period / 1000000.0);
  printf("GPU time: %fms\n", runtime_ms);

  core::Mat<float, 1> mat_blur(3000, 4000);
  std::vector<float> gaussian_kernel = {0.0625f, 0.125f,  0.0625f, 0.125f, 0.25f,
                                        0.125f,  0.0625f, 0.125f,  0.0625f};
  t.start();
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
  t.end();
  printf("CPU time: %fms\n", t.time());

  // check data
  core::Mat<float, 1> blur_cpu(3000, 4000);
  dst_buffer.MapData(
      [&blur_cpu](void* data) { memcpy(blur_cpu.data(), data, sizeof(float) * blur_cpu.total()); });

  for (int row = 0; row < blur_cpu.rows(); ++row) {
    for (int col = 0; col < blur_cpu.cols(); ++col) {
      EXPECT_EQ(*blur_cpu(row, col), *mat_blur(row, col));
    }
  }
}

}  // namespace test
}  // namespace core