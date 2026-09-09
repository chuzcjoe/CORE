#pragma once

#define GLM_FORCE_RADIANS
#include <chrono>
#include <vector>

#include "VulkanBuffer.h"
#include "VulkanRender.h"
#include "VulkanUtils.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace core {

class RenderGalaxy : public core::vulkan::VulkanRender {
 public:
  RenderGalaxy(core::vulkan::VulkanContext* context,
               const core::vulkan::DynamicRenderingInfo& dynamic_rendering_info,
               uint32_t star_count);

  void Init() override;
  void Render(VkCommandBuffer command_buffer, VkExtent2D extent);

  void UpdateUniformBuffer(uint32_t width, uint32_t height);

 protected:
  // Points/sprites — no triangle culling
  VkCullModeFlags SetCullMode() const override { return VK_CULL_MODE_NONE; }
  VkFrontFace SetFrontFace() const override { return VK_FRONT_FACE_COUNTER_CLOCKWISE; }
  VkPipelineColorBlendAttachmentState SetColorBlendAttachment() const override;

  std::vector<core::vulkan::BindingInfo> GetBindingInfo() const override;
  const std::vector<uint32_t> LoadVertexShader() const override;
  const std::vector<uint32_t> LoadFragmentShader() const override;
  std::vector<VkVertexInputBindingDescription> GetVertexBindingDescriptions() const override;
  std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() const override;

 private:
  struct InstanceData {
    glm::vec3 pos;
    glm::vec3 color;
    float size;
  };

  struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 project;
    float time;
    float aspect;
    float pad0;
    float pad1;
  } uniform_data_;

  void GenerateStars();
  void CreateBuffers();

  uint32_t star_count_;
  std::vector<InstanceData> instances_;

  // Unit quad — 4 corners, indices form 2 triangles
  const std::vector<glm::vec2> quad_corners_ = {
      {-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};
  const std::vector<uint16_t> quad_indices_ = {0, 1, 2, 2, 3, 0};

  core::vulkan::VulkanBuffer quad_buffer_staging_;
  core::vulkan::VulkanBuffer quad_buffer_local_;
  core::vulkan::VulkanBuffer index_buffer_staging_;
  core::vulkan::VulkanBuffer index_buffer_local_;
  core::vulkan::VulkanBuffer instance_buffer_staging_;
  core::vulkan::VulkanBuffer instance_buffer_local_;
  core::vulkan::VulkanBuffer uniform_buffer_;

  inline static std::chrono::time_point<std::chrono::high_resolution_clock> start_time_ =
      std::chrono::high_resolution_clock::now();
};

}  // namespace core
