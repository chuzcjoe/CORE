#pragma once

#include <vector>

#include "VulkanBuffer.h"
#include "VulkanRender.h"
#include "VulkanUtils.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

namespace core {

// Per-instance transform + color for one cube.
struct CubeInstance {
  glm::mat4 model;
  glm::vec3 color;
};

// Instanced renderer for shaded unit cubes. A single mesh is drawn many times,
// once per CubeInstance supplied through SetInstances(). Used for both the
// shootable targets and the in-flight bullets.
class RenderCubes : public core::vulkan::VulkanRender {
 public:
  RenderCubes(core::vulkan::VulkanContext* context,
              const core::vulkan::DynamicRenderingInfo& dynamic_rendering_info,
              uint32_t max_instances);

  void Init() override;
  void Render(VkCommandBuffer command_buffer, VkExtent2D extent);

  void UpdateUniformBuffer(const glm::mat4& view, const glm::mat4& project);
  // Upload the instances to draw this frame (clamped to max_instances).
  void SetInstances(const std::vector<CubeInstance>& instances);

 protected:
  // Targets tumble freely, so don't cull either winding.
  VkCullModeFlags SetCullMode() const override { return VK_CULL_MODE_NONE; }
  VkFrontFace SetFrontFace() const override { return VK_FRONT_FACE_COUNTER_CLOCKWISE; }
  VkBool32 SetDepthTesting() const override { return VK_TRUE; }
  VkBool32 SetDepthWriting() const override { return VK_TRUE; }

  std::vector<core::vulkan::BindingInfo> GetBindingInfo() const override;
  const std::vector<uint32_t> LoadVertexShader() const override;
  const std::vector<uint32_t> LoadFragmentShader() const override;
  std::vector<VkVertexInputBindingDescription> GetVertexBindingDescriptions() const override;
  std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() const override;

 private:
  struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
  };

  struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 project;
  };

  void BuildGeometry();
  void CreateBuffers();

  uint32_t max_instances_ = 0;
  uint32_t instance_count_ = 0;

  std::vector<Vertex> vertices_;
  std::vector<uint16_t> indices_;

  core::vulkan::VulkanBuffer vertex_buffer_staging_;
  core::vulkan::VulkanBuffer vertex_buffer_local_;
  core::vulkan::VulkanBuffer index_buffer_staging_;
  core::vulkan::VulkanBuffer index_buffer_local_;
  // Host-visible so the instance transforms can be re-uploaded every frame.
  core::vulkan::VulkanBuffer instance_buffer_;
  core::vulkan::VulkanBuffer uniform_buffer_;
};

}  // namespace core
