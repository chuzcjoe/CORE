#pragma once

#include <vector>

#include "VulkanGraphic.h"
#include "VulkanRenderPass.h"
#include "VulkanUtils.h"

namespace core {

class GraphicTriangle : public core::vulkan::VulkanGraphic {
 public:
  GraphicTriangle(core::vulkan::VulkanContext* context,
                  core::vulkan::VulkanRenderPass& render_pass);

  void Init() override;
  void Render(VkCommandBuffer command_buffer, VkExtent2D extent);

 protected:
  std::vector<core::vulkan::BindingInfo> GetBindingInfo() const override;
  const std::vector<uint32_t> LoadVertexShader() const override;
  const std::vector<uint32_t> LoadFragmentShader() const override;
  std::array<VkVertexInputBindingDescription, 1> GetVertexBindingDescriptions() const override;
  std::array<VkVertexInputAttributeDescription, 2> GetVertexAttributeDescriptions() const override;

 private:
  struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
  };

  void CreateVertexBuffer();

  // pos, color
  const std::vector<Vertex> vertices_ = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                         {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                         {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                         {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
  // index buffer
  const std::vector<uint16_t> indices_ = {0, 1, 2, 2, 3, 0};
  core::vulkan::VulkanBuffer vertex_buffer_staging_;
  core::vulkan::VulkanBuffer vertex_buffer_local_;

  core::vulkan::VulkanBuffer index_buffer_staging_;
  core::vulkan::VulkanBuffer index_buffer_local_;
};

}  // namespace core
