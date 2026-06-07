#pragma once

#include <array>
#include <string>
#include <vector>

#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "VulkanRender.h"
#include "VulkanSampler.h"
#include "VulkanUtils.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

namespace core {

// Renders a sky cube from six face images stitched into a Vulkan cubemap.
// Drawn first each frame with depth testing disabled so it sits behind every
// other object.
class RenderSkybox : public core::vulkan::VulkanRender {
 public:
  RenderSkybox(core::vulkan::VulkanContext* context,
               const core::vulkan::DynamicRenderingInfo& dynamic_rendering_info);

  void Init() override;
  // Faces must be supplied in Vulkan cubemap layer order: +X, -X, +Y, -Y, +Z, -Z
  // (i.e. right, left, top, bottom, front, back).
  void Init(const std::array<std::string, 6>& face_paths);
  void Render(VkCommandBuffer command_buffer, VkExtent2D extent);

  // Pass the camera view matrix; the translation component is stripped so the
  // skybox stays centered on the camera.
  void UpdateUniformBuffer(const glm::mat4& view, const glm::mat4& project);

 protected:
  VkCullModeFlags SetCullMode() const override { return VK_CULL_MODE_NONE; }
  VkFrontFace SetFrontFace() const override { return VK_FRONT_FACE_COUNTER_CLOCKWISE; }
  VkBool32 SetDepthTesting() const override { return VK_FALSE; }
  VkBool32 SetDepthWriting() const override { return VK_FALSE; }

  std::vector<core::vulkan::BindingInfo> GetBindingInfo() const override;
  const std::vector<uint32_t> LoadVertexShader() const override;
  const std::vector<uint32_t> LoadFragmentShader() const override;
  std::vector<VkVertexInputBindingDescription> GetVertexBindingDescriptions() const override;
  std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() const override;

 private:
  struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 project;
  };

  void LoadCubeMap(const std::array<std::string, 6>& face_paths);
  void CreateBuffers();

  core::vulkan::VulkanBuffer vertex_buffer_staging_;
  core::vulkan::VulkanBuffer vertex_buffer_local_;
  core::vulkan::VulkanBuffer uniform_buffer_;
  core::vulkan::VulkanImage cube_map_image_;
  core::vulkan::VulkanSampler sampler_;

  // 36 vertices (6 faces × 2 triangles × 3 verts) of a unit cube, positions only.
  // clang-format off
  const std::vector<float> skybox_vertices_ = {
       1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,
      -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
  };
  // clang-format on
};

}  // namespace core
