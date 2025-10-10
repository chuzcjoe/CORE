#pragma once

#include <iostream>

#include "Mat.h"
#include "VulkanBuffer.h"
#include "VulkanCompute.h"
#include "glm/glm.hpp"

namespace core {
namespace example {

class ComputeBarycentric : public core::vulkan::VulkanCompute {
 public:
  ComputeBarycentric(core::vulkan::VulkanContext* context, const int width, const int height);

  void Init() override;
  void Run(const VkCommandBuffer command_buffer);

  core::vulkan::VulkanBuffer rasterized;

 protected:
  std::vector<core::vulkan::BindingInfo> GetBindingInfo() const override;
  const std::vector<uint32_t>& LoadShaderCode() const override;

 private:
  void CreateBuffers();

  core::vulkan::VulkanBuffer uniform_buffer_;
  struct UniformData {
    int width;
    int height;
    glm::vec2 vertex_a;
    glm::vec2 vertex_b;
    glm::vec2 vertex_c;
  } uniform_data_;
};
}  // namespace example
}  // namespace core