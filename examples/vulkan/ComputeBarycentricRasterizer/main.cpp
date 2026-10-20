#include "Status.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "ComputeBarycentric.h"
#include "Mat.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanQueryPool.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

const int kWidth = 1000;
const int kHeight = 1000;

core::Status RunApplication() {
  // Setup Vulkan
  core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Compute;

  auto context_result =
      core::vulkan::VulkanContext::Create(true, queue_family_type, nullptr);
  if (!context_result.ok()) {
    return context_result.status();
  }
  std::unique_ptr<core::vulkan::VulkanContext> context_owner =
      std::move(*context_result);
  core::vulkan::VulkanContext& context = *context_owner;
  {
    const core::Status status = context.Init();
    if (!status.ok()) {
      return status;
    }
  }

  auto command_buffer_result =
      core::vulkan::VulkanCommandBuffer::Create(&context);
  if (!command_buffer_result.ok()) {
    return command_buffer_result.status();
  }
  std::unique_ptr<core::vulkan::VulkanCommandBuffer> command_buffer_owner =
      std::move(*command_buffer_result);
  core::vulkan::VulkanCommandBuffer& command_buffer = *command_buffer_owner;

  auto fence_result = core::vulkan::VulkanFence::Create(&context);
  if (!fence_result.ok()) {
    return fence_result.status();
  }
  std::unique_ptr<core::vulkan::VulkanFence> fence_owner =
      std::move(*fence_result);
  core::vulkan::VulkanFence& fence = *fence_owner;

  auto query_pool_result =
      core::vulkan::VulkanQueryPool::Create(&context, VK_QUERY_TYPE_TIMESTAMP);
  if (!query_pool_result.ok()) {
    return query_pool_result.status();
  }
  std::unique_ptr<core::vulkan::VulkanQueryPool> query_pool_owner =
      std::move(*query_pool_result);
  core::vulkan::VulkanQueryPool& query_pool = *query_pool_owner;

  std::unique_ptr<core::example::ComputeBarycentric> barycentric =
      std::make_unique<core::example::ComputeBarycentric>(&context, kWidth, kHeight);
  {
    const core::Status status = barycentric->Init();
    if (!status.ok()) {
      return status;
    }
  }

  {
    const core::Status status = fence.Reset();
    if (!status.ok()) {
      return status;
    }
  }

  {
    const core::Status status = command_buffer.BeginCommandBufferWithReset();
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = query_pool.Reset(command_buffer.buffer());
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = query_pool.Query(
        command_buffer.buffer(), 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = barycentric->Run(command_buffer.buffer());
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = query_pool.Query(
        command_buffer.buffer(), 1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status =
        command_buffer.Submit(fence.fence, core::vulkan::SubmitSyncInfo{});
    if (!status.ok()) {
      return status;
    }
  }

  vkWaitForFences(context.logical_device, 1, &fence.fence, VK_TRUE, UINT64_MAX);

  {
    const core::Status status = query_pool.GetQueryResults();
    if (!status.ok()) {
      return status;
    }
  }
  const auto timestamps = query_pool.GetTimeStamps();
  const auto runtime_ms = (timestamps[1] - timestamps[0]) * (context.timestamp_period / 1000000.0);
  printf("GPU time: %fms\n", runtime_ms);

  core::Mat<int, 1> result(kHeight, kWidth);
  {
    const core::Status status =
        barycentric->rasterized.MapData([&result](void* data) {
          memcpy(result.data(), data, result.total() * sizeof(int));
        });
    if (!status.ok()) {
      return status;
    }
  }

  printf("rasterization at (1, 1): %d\n", *result(1, 1));
  printf("rasterization at (500, 600): %d\n", *result(500, 600));

  std::vector<unsigned char> pixels(kWidth * kHeight);
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      pixels[y * kWidth + x] = static_cast<unsigned char>(*result(y, x) ? 255 : 0);
    }
  }

  const bool write_status =
      stbi_write_png("./rasterized.png", kWidth, kHeight, 1, pixels.data(), kWidth);
  if (write_status) {
    printf("saved rasterization results successfully");
  } else {
    return core::Status(core::ErrorCode::kBackend,
                        "could not save rasterized results");
  }

  return core::Status();
}

int main() {
  const core::Status status = RunApplication();
  if (!status.ok()) {
    std::fprintf(stderr, "%s\n", status.message().c_str());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
