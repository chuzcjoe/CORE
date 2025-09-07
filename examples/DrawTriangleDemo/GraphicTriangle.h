#pragma once

#include <vector>

#include "VulkanGraphic.h"
#include "VulkanRenderPass.h"
#include "VulkanUtils.h"

namespace core {

class GraphicTriangle : public core::vulkan::VulkanGraphic {
 public:
  GraphicTriangle(core::vulkan::VulkanContext* context, core::vulkan::VulkanRenderPass render_pass);

  void Init() override;
  void Render(VkCommandBuffer command_buffer);

 protected:
  std::vector<core::vulkan::BindingInfo> GetBindingInfo() const override;
  const std::vector<uint32_t> LoadVertexShader() const override;
  const std::vector<uint32_t> LoadFragmentShader() const override;
  std::vector<VkVertexInputBindingDescription> GetVertexBindingDescriptions() const override;
  std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() const override;

 private:
  void CreateVertexBuffer();
  // pos, color
  const std::vector<core::vulkan::Vertex> vertices_ = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                                       {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                                       {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};

  core::vulkan::VulkanBuffer vertex_buffer_;
};

}  // namespace core
