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

int main() {
  // Setup Vulkan
  core::vulkan::QueueFamilyType queue_family_type = core::vulkan::QueueFamilyType::Compute;
  core::vulkan::VulkanContext context(true, queue_family_type, nullptr);
  context.Init();
  core::vulkan::VulkanCommandBuffer command_buffer(&context);
  core::vulkan::VulkanFence fence(&context);
  core::vulkan::VulkanQueryPool query_pool(&context, VK_QUERY_TYPE_TIMESTAMP);

  std::unique_ptr<core::example::ComputeBarycentric> barycentric =
      std::make_unique<core::example::ComputeBarycentric>(&context, kWidth, kHeight);
  barycentric->Init();

  fence.Reset();

  vkResetCommandBuffer(command_buffer.buffer(), 0);
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  vkBeginCommandBuffer(command_buffer.buffer(), &begin_info);
  query_pool.Reset(command_buffer.buffer());
  query_pool.Query(command_buffer.buffer(), 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  barycentric->Run(command_buffer.buffer());
  query_pool.Query(command_buffer.buffer(), 1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  VkSubmitInfo submit_info{};
  command_buffer.Submit(fence.fence, submit_info);

  vkWaitForFences(context.logical_device, 1, &fence.fence, VK_TRUE, UINT64_MAX);

  query_pool.GetQueryResults();
  const auto timestamps = query_pool.GetTimeStamps();
  const auto runtime_ms = (timestamps[1] - timestamps[0]) * (context.timestamp_period / 1000000.0);
  printf("GPU time: %fms\n", runtime_ms);

  core::Mat<int, 1> result(kHeight, kWidth);
  barycentric->rasterized.MapData(
      [&result](void* data) { memcpy(result.data(), data, result.total() * sizeof(int)); });

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
    throw std::runtime_error("could not save rasterized results");
  }

  return 0;
}
