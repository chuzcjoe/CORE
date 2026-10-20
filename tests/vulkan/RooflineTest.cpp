#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <memory>

#include "ComputeCopy.h"
#include "ComputeFma.h"
#include "Status.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanQueryPool.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

namespace core {
namespace test {
namespace {

// Records `record` into a fresh command buffer bracketed by GPU timestamps,
// submits it, waits for completion, and returns the GPU time in milliseconds.
Result<double> TimeDispatchMs(
    core::vulkan::VulkanContext& context,
    core::vulkan::VulkanCommandBuffer& command_buffer,
    core::vulkan::VulkanFence& fence, core::vulkan::VulkanQueryPool& query_pool,
    const std::function<Status(VkCommandBuffer)>& record) {
  {
    const core::Status status = fence.Reset();
    if (!status.ok()) {
      return Result<double>(status);
    }
  }
  {
    const core::Status status = command_buffer.BeginCommandBufferWithReset();
    if (!status.ok()) {
      return Result<double>(status);
    }
  }

  {
    const core::Status status = query_pool.Reset(command_buffer.buffer());
    if (!status.ok()) {
      return Result<double>(status);
    }
  }
  {
    const core::Status status = query_pool.Query(
        command_buffer.buffer(), 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    if (!status.ok()) {
      return Result<double>(status);
    }
  }
  const Status record_status = record(command_buffer.buffer());
  if (!record_status.ok()) {
    return Result<double>(record_status);
  }
  {
    const core::Status status = query_pool.Query(
        command_buffer.buffer(), 1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    if (!status.ok()) {
      return Result<double>(status);
    }
  }

  VkSubmitInfo submit_info{};
  {
    const core::Status status = command_buffer.Submit(fence.fence, submit_info);
    if (!status.ok()) {
      return Result<double>(status);
    }
  }
  vkWaitForFences(context.logical_device, 1, &fence.fence, VK_TRUE, UINT64_MAX);

  {
    const core::Status status = query_pool.GetQueryResults();
    if (!status.ok()) {
      return Result<double>(status);
    }
  }
  const auto timestamps = query_pool.GetTimeStamps();
  return Result<double>((timestamps[1] - timestamps[0]) *
                        (context.timestamp_period / 1000000.0));
}

}  // namespace

// Measures the two roofline-model ceilings with microbenchmarks and derives
// the ridge point (peak compute throughput / peak memory bandwidth). Kernels
// with arithmetic intensity below the ridge point are memory-bound, above it
// compute-bound.
TEST(Roofline, RidgePoint) {
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

  // Early runs also serve as warm-up while DVFS ramps the GPU clock up; the
  // minimum over all runs is the peak.
  constexpr int kRuns = 6;

  // ---- Peak compute throughput: register-resident FMA chains ----
  constexpr uint32_t kFmaGroups = 2048;
  constexpr uint32_t kFmaThreads = kFmaGroups * core::vulkan::ComputeFma::kLocalSizeX;
  constexpr int kFmaIterations = 1024;

  auto fma_dst_result = core::vulkan::VulkanBuffer::Create(
      &context, kFmaThreads * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!fma_dst_result.ok()) {
    FAIL() << fma_dst_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanBuffer> fma_dst_owner =
      std::move(*fma_dst_result);
  core::vulkan::VulkanBuffer& fma_dst = *fma_dst_owner;

  auto fma =
      std::make_unique<core::vulkan::ComputeFma>(&context, fma_dst, kFmaIterations, kFmaGroups);
  {
    const core::Status status = fma->Init();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  double fma_ms = std::numeric_limits<double>::max();
  for (int run = 0; run < kRuns; ++run) {
    auto elapsed =
        TimeDispatchMs(context, command_buffer, fence, query_pool,
                       [&](VkCommandBuffer cmd) { return fma->Run(cmd); });
    ASSERT_TRUE(elapsed.ok()) << elapsed.status().message();
    fma_ms = std::min(fma_ms, *elapsed);
  }

  const double total_flop = static_cast<double>(kFmaThreads) * kFmaIterations *
                            core::vulkan::ComputeFma::kFlopPerIteration;
  const double peak_gflops = total_flop / (fma_ms * 1e6);

  // The chains converge toward 0.1; a finite result proves the loop ran.
  float fma_sample = 0.0f;
  {
    const core::Status status = fma_dst.MapData([&fma_sample](void* data) {
      memcpy(&fma_sample, data, sizeof(float));
    });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  EXPECT_TRUE(std::isfinite(fma_sample));

  // ---- Peak memory bandwidth: streaming vec4 copy ----
  // 8M vec4 = 128 MiB per buffer, far larger than the GPU's caches.
  constexpr int kCopyCount = 8 * 1024 * 1024;
  constexpr VkDeviceSize kCopyBytes = static_cast<VkDeviceSize>(kCopyCount) * 16;

  auto copy_src_result = core::vulkan::VulkanBuffer::Create(
      &context, kCopyBytes,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (!copy_src_result.ok()) {
    FAIL() << copy_src_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanBuffer> copy_src_owner =
      std::move(*copy_src_result);
  core::vulkan::VulkanBuffer& copy_src = *copy_src_owner;

  auto copy_dst_result = core::vulkan::VulkanBuffer::Create(
      &context, kCopyBytes,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (!copy_dst_result.ok()) {
    FAIL() << copy_dst_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanBuffer> copy_dst_owner =
      std::move(*copy_dst_result);
  core::vulkan::VulkanBuffer& copy_dst = *copy_dst_owner;

  auto staging_result = core::vulkan::VulkanBuffer::Create(
      &context, kCopyBytes,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!staging_result.ok()) {
    FAIL() << staging_result.status().message();
  }
  std::unique_ptr<core::vulkan::VulkanBuffer> staging_owner =
      std::move(*staging_result);
  core::vulkan::VulkanBuffer& staging = *staging_owner;

  {
    const core::Status status = staging.MapData([](void* data) {
      float* floats = static_cast<float*>(data);
      for (int i = 0; i < kCopyCount * 4; ++i) {
        floats[i] = static_cast<float>(i % 1024);
      }
    });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = staging.CopyToBuffer(copy_src);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  auto copy = std::make_unique<core::vulkan::ComputeCopy>(&context, copy_src, copy_dst, kCopyCount);
  {
    const core::Status status = copy->Init();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  double copy_ms = std::numeric_limits<double>::max();
  for (int run = 0; run < kRuns; ++run) {
    auto elapsed =
        TimeDispatchMs(context, command_buffer, fence, query_pool,
                       [&](VkCommandBuffer cmd) { return copy->Run(cmd); });
    ASSERT_TRUE(elapsed.ok()) << elapsed.status().message();
    copy_ms = std::min(copy_ms, *elapsed);
  }

  const double bytes_moved = 2.0 * static_cast<double>(kCopyBytes);  // read + write
  const double peak_gbps = bytes_moved / (copy_ms * 1e6);

  // Verify the copy actually happened before trusting the bandwidth number.
  {
    const core::Status status = copy_dst.CopyToBuffer(staging);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  float first = -1.0f, middle = -1.0f, last = -1.0f;
  {
    const core::Status status =
        staging.MapData([&first, &middle, &last](void* data) {
          const float* floats = static_cast<const float*>(data);
          constexpr int kFloatCount = kCopyCount * 4;
          first = floats[0];
          middle = floats[kFloatCount / 2];
          last = floats[kFloatCount - 1];
        });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  EXPECT_FLOAT_EQ(first, 0.0f);
  EXPECT_FLOAT_EQ(middle, static_cast<float>((kCopyCount * 4 / 2) % 1024));
  EXPECT_FLOAT_EQ(last, static_cast<float>((kCopyCount * 4 - 1) % 1024));

  // ---- Ridge point ----
  const double ridge_point = peak_gflops / peak_gbps;  // FLOP per byte

  printf("Peak compute throughput: %.1f GFLOP/s (fp32, best of %d runs, %.3f ms)\n", peak_gflops,
         kRuns, fma_ms);
  printf("Peak memory bandwidth  : %.1f GB/s (best of %d runs, %.3f ms)\n", peak_gbps, kRuns,
         copy_ms);
  printf("Ridge point            : %.1f FLOP/byte\n", ridge_point);

  EXPECT_GT(peak_gflops, 0.0);
  EXPECT_GT(peak_gbps, 0.0);
}

}  // namespace test
}  // namespace core
