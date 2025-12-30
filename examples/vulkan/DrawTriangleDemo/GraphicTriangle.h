#pragma once

#define GLM_FORCE_RADIANS
#include <chrono>
#include <vector>

#include "VulkanGraphic.h"
#include "VulkanRenderPass.h"
#include "VulkanUtils.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace core {

class GraphicTriangle : public core::vulkan::VulkanGraphic {
 public:
  GraphicTriangle(core::vulkan::VulkanContext* context,
                  core::vulkan::VulkanRenderPass* render_pass);

  GraphicTriangle(core::vulkan::VulkanContext* context,
                  const core::vulkan::DynamicRenderingInfo& dynamic_rendering_info);

  void Init() override;
  void Render(VkCommandBuffer command_buffer, VkExtent2D extent);

  void UpdateUniformBuffer(const int width, const int height);

 protected:
  VkCullModeFlags SetCullMode() const override { return VK_CULL_MODE_BACK_BIT; }
  VkFrontFace SetFrontFace() const override { return VK_FRONT_FACE_COUNTER_CLOCKWISE; }

  std::vector<core::vulkan::BindingInfo> GetBindingInfo() const override;
  const std::vector<uint32_t> LoadVertexShader() const override;
  const std::vector<uint32_t> LoadFragmentShader() const override;
  std::vector<VkVertexInputBindingDescription> GetVertexBindingDescriptions() const override;
  std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() const override;

 private:
  struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
  };

  struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 project;
  } uniform_data_;

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

  // uniform buffer
  core::vulkan::VulkanBuffer uniform_buffer_;

  // start time, we need it to calculate the rotation angle
  inline static std::chrono::time_point<std::chrono::high_resolution_clock> start_time_ =
      std::chrono::high_resolution_clock::now();
};

}  // namespace core
