#pragma once

#include <iostream>

#include "VulkanBuffer.h"
#include "VulkanCompute.h"
#include "glm/glm.hpp"

namespace core {
namespace example {

class ComputeBarycentric : public core::vulkan::VulkanCompute {
 public:
  struct Triangle {
    glm::ivec2 a;
    glm::ivec2 b;
    glm::ivec2 c;
  };

  ComputeBarycentric(core::vulkan::VulkanContext* context, const int width, const int height);

  void Init() override;
  void Run(const VkCommandBuffer command_buffer);
  void Run(VkCommandBuffer command_buffer, const Triangle& triangle);

  core::vulkan::VulkanBuffer rasterized;

 protected:
  std::vector<core::vulkan::BindingInfo> GetBindingInfo() const override;
  std::vector<VkPushConstantRange> GetPushConstantRanges() const override;
  const std::vector<uint32_t>& LoadShaderCode() const override;

 private:
  void CreateBuffers();

  struct PushConstants {
    int width;
    int height;
    glm::ivec2 vertex_a;
    glm::ivec2 vertex_b;
    glm::ivec2 vertex_c;
  };
  static_assert(sizeof(PushConstants) == 32);

  int width_;
  int height_;
};
}  // namespace example
}  // namespace core