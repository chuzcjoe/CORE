#pragma once

#include <chrono>
#include <vector>

#include "VulkanGraphic.h"
#include "VulkanImage.h"
#include "VulkanRenderPass.h"
#include "VulkanSampler.h"
#include "VulkanUtils.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace core {

class GraphicDepth : public core::vulkan::VulkanGraphic {
 public:
  GraphicDepth(core::vulkan::VulkanContext* context, core::vulkan::VulkanRenderPass& render_pass);

  void Init() override;
  void Init(const std::string& image_path, VkExtent2D extent);
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
  void CreateTextureImage(const std::string& image_path);
  void CreateDepthImage();

  VkFormat GetDepthFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                          VkFormatFeatureFlags features) const;

  struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 tex_coord;
  };

  struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 project;
  } uniform_data_;

  void CreateBuffers();

  // pos, color, text_coord
  const std::vector<Vertex> vertices_ = {{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                                         {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                                         {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                                         {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

                                         {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                                         {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                                         {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                                         {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}};
  // index buffer
  const std::vector<uint16_t> indices_ = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};
  core::vulkan::VulkanBuffer vertex_buffer_staging_;
  core::vulkan::VulkanBuffer vertex_buffer_local_;

  core::vulkan::VulkanBuffer index_buffer_staging_;
  core::vulkan::VulkanBuffer index_buffer_local_;

  // uniform buffer
  core::vulkan::VulkanBuffer uniform_buffer_;

  // texture image
  core::vulkan::VulkanImage texture_image_;
  core::vulkan::VulkanSampler sampler_;

  // depth image
  core::vulkan::VulkanImage depth_image_;
};

}  // namespace core
