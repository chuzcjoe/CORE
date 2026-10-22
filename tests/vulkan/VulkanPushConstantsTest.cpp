#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ComputeBarycentric.h"
#include "VulkanBase.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanSync.h"

namespace core {
namespace test {
namespace {

using Triangle = example::ComputeBarycentric::Triangle;

class LayoutProbe : public vulkan::VulkanBase {
 public:
  LayoutProbe(vulkan::VulkanContext* context,
              std::vector<VkPushConstantRange> ranges)
      : VulkanBase(context), ranges_(std::move(ranges)) {}

 private:
  std::vector<vulkan::BindingInfo> GetBindingInfo() const override {
    return {};
  }
  std::vector<VkPushConstantRange> GetPushConstantRanges() const override {
    return ranges_;
  }
  void CreatePipeline() override {}

  std::vector<VkPushConstantRange> ranges_;
};

bool Covers(const Triangle& triangle, int x, int y) {
  if (x < std::min({triangle.a.x, triangle.b.x, triangle.c.x}) ||
      x > std::max({triangle.a.x, triangle.b.x, triangle.c.x}) ||
      y < std::min({triangle.a.y, triangle.b.y, triangle.c.y}) ||
      y > std::max({triangle.a.y, triangle.b.y, triangle.c.y})) {
    return false;
  }
  const glm::ivec2 vertices[] = {triangle.a, triangle.b, triangle.c,
                                 triangle.a};
  for (int edge = 0; edge < 3; ++edge) {
    const int64_t dx =
        static_cast<int64_t>(vertices[edge + 1].x) - vertices[edge].x;
    const int64_t dy =
        static_cast<int64_t>(vertices[edge + 1].y) - vertices[edge].y;
    const int64_t px = static_cast<int64_t>(x) - vertices[edge].x;
    const int64_t py = static_cast<int64_t>(y) - vertices[edge].y;
    if (dx * py - dy * px < 0) {
      return false;
    }
  }
  return true;
}

void SubmitAndWait(vulkan::VulkanContext& context,
                   vulkan::VulkanCommandBuffer& command) {
  VkMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(command.buffer(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr,
                       0, nullptr);
  vulkan::VulkanFence fence(&context);
  fence.Reset();
  command.Submit(fence.fence, vulkan::SubmitSyncInfo{});
  VK_CHECK(vkWaitForFences(context.logical_device, 1, &fence.fence, VK_TRUE,
                           UINT64_MAX));
}

TEST(VulkanPushConstantsTest,
     AcceptsDeviceLimitAndSharedBytesForDistinctStages) {
  vulkan::VulkanContext context(true);
  context.Init();
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(context.physical_device, &properties);
  LayoutProbe probe(
      &context,
      {{.stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = 4},
       {.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = 4},
       {.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = properties.limits.maxPushConstantsSize - 4,
        .size = 4}});
  probe.Init();
  EXPECT_NE(probe.pipeline_layout, VK_NULL_HANDLE);
}

TEST(VulkanPushConstantsTest, RejectsInvalidRangesBeforeCreatingResources) {
  vulkan::VulkanContext context(true);
  context.Init();
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(context.physical_device, &properties);
  const uint32_t limit = properties.limits.maxPushConstantsSize;
  const std::vector<VkPushConstantRange> invalid_ranges = {
      {.stageFlags = 0, .offset = 0, .size = 4},
      {.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 2, .size = 4},
      {.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = 0},
      {.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = 6},
      {.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = limit, .size = 4},
      {.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
       .offset = limit - 4,
       .size = 8},
      {.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
       .offset = UINT32_MAX - 3,
       .size = 4}};
  for (const VkPushConstantRange& range : invalid_ranges) {
    LayoutProbe probe(&context, {range});
    EXPECT_THROW(probe.Init(), std::invalid_argument);
    EXPECT_EQ(probe.pipeline_layout, VK_NULL_HANDLE);
  }
  LayoutProbe duplicate(
      &context,
      {{.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = 4},
       {.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 4, .size = 4}});
  EXPECT_THROW(duplicate.Init(), std::invalid_argument);
}

TEST(VulkanPushConstantsTest,
     RecordsDistinctTrianglesAndReusesPipelineAcrossSubmissions) {
  vulkan::VulkanContext context(true);
  context.Init();
  example::ComputeBarycentric rasterizer(&context, 37, 29);
  rasterizer.Init();
  vulkan::VulkanCommandBuffer command(&context);
  const std::vector<Triangle> triangles = {
      {.a = glm::ivec2(1, 1), .b = glm::ivec2(21, 2), .c = glm::ivec2(3, 25)},
      {.a = glm::ivec2(10, 8), .b = glm::ivec2(36, 8), .c = glm::ivec2(30, 28)},
      {.a = glm::ivec2(-5, -3),
       .b = glm::ivec2(12, -3),
       .c = glm::ivec2(-5, 15)}};
  for (size_t count = 1; count <= triangles.size(); ++count) {
    rasterizer.rasterized.MapData([&rasterizer](void* data) {
      std::memset(data, 0, static_cast<size_t>(rasterizer.rasterized.Size()));
    });
    command.BeginCommandBufferWithReset();
    for (size_t index = 0; index < count; ++index) {
      Triangle parameters = triangles[index];
      rasterizer.Run(command.buffer(), parameters);
      parameters.a = glm::ivec2(100, 100);
      parameters.b = glm::ivec2(110, 100);
      parameters.c = glm::ivec2(100, 110);
    }
    SubmitAndWait(context, command);
    rasterizer.rasterized.MapData([&triangles, count](void* data) {
      const int* pixels = static_cast<const int*>(data);
      for (int y = 0; y < 29; ++y) {
        for (int x = 0; x < 37; ++x) {
          bool covered = false;
          for (size_t index = 0; index < count; ++index) {
            covered = covered || Covers(triangles[index], x, y);
          }
          ASSERT_EQ(pixels[y * 37 + x], covered ? 1 : 0) << x << "," << y;
        }
      }
    });
  }
}

TEST(VulkanPushConstantsTest,
     PreservesDefaultTriangleAndHandlesSinglePixelAndErrors) {
  vulkan::VulkanContext context(true);
  context.Init();
  example::ComputeBarycentric original(&context, 1000, 1000);
  original.Init();
  vulkan::VulkanCommandBuffer command(&context);
  command.BeginCommandBufferWithReset();
  original.Run(command.buffer());
  SubmitAndWait(context, command);
  const Triangle legacy{.a = glm::ivec2(300, 800),
                        .b = glm::ivec2(500, 100),
                        .c = glm::ivec2(700, 800)};
  original.rasterized.MapData([&legacy](void* data) {
    const int* pixels = static_cast<const int*>(data);
    for (int y = 0; y < 1000; ++y) {
      for (int x = 0; x < 1000; ++x) {
        ASSERT_EQ(pixels[y * 1000 + x], Covers(legacy, x, y) ? 1 : 0);
      }
    }
  });

  EXPECT_THROW((example::ComputeBarycentric(&context, 0, 1)),
               std::invalid_argument);
  EXPECT_THROW((example::ComputeBarycentric(&context, 1, 16385)),
               std::invalid_argument);
  example::ComputeBarycentric single(&context, 1, 1);
  single.Init();
  command.BeginCommandBufferWithReset();
  Triangle point{.a = glm::ivec2(0), .b = glm::ivec2(0), .c = glm::ivec2(0)};
  point.a.x = 16384;
  EXPECT_THROW(single.Run(command.buffer(), point), std::invalid_argument);
  point.a.x = -16384;
  EXPECT_THROW(single.Run(command.buffer(), point), std::invalid_argument);
  point.a.x = 0;
  single.Run(command.buffer(), point);
  SubmitAndWait(context, command);
  single.rasterized.MapData(
      [](void* data) { EXPECT_EQ(*static_cast<const int*>(data), 1); });

  single.rasterized.MapData([](void* data) { *static_cast<int*>(data) = 0; });
  command.BeginCommandBufferWithReset();
  single.Run(command.buffer(), Triangle{.a = glm::ivec2(-16383, -16383),
                                        .b = glm::ivec2(16383, -16383),
                                        .c = glm::ivec2(-16383, 16383)});
  SubmitAndWait(context, command);
  single.rasterized.MapData(
      [](void* data) { EXPECT_EQ(*static_cast<const int*>(data), 1); });
}

}
}
}
