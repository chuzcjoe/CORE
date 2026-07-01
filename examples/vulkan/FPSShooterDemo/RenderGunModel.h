#pragma once

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

// First-person view model that renders a real textured weapon mesh (loaded from
// an OBJ + colormap texture) glued to the screen in view space. Also draws a
// small screen-center crosshair in the same pipeline.
class RenderGunModel : public core::vulkan::VulkanRender {
 public:
  RenderGunModel(core::vulkan::VulkanContext* context,
                 const core::vulkan::DynamicRenderingInfo& dynamic_rendering_info);

  // Loads the texture + OBJ mesh, then sets up GPU resources.
  void Init(const std::string& image_path, const std::string& model_path);
  void Render(VkCommandBuffer command_buffer, VkExtent2D extent);

  // viewmodel places the weapon in camera space (recoil already baked in);
  // flash is the 0..1 muzzle-flash intensity this frame.
  void UpdateUniformBuffer(const glm::mat4& project, const glm::mat4& viewmodel, float flash);

 protected:
  // The mesh is close to the camera, so normal depth testing keeps it on top of
  // the world while still self-occluding correctly.
  VkCullModeFlags SetCullMode() const override { return VK_CULL_MODE_NONE; }
  VkFrontFace SetFrontFace() const override { return VK_FRONT_FACE_COUNTER_CLOCKWISE; }
  VkBool32 SetDepthTesting() const override { return VK_TRUE; }
  VkBool32 SetDepthWriting() const override { return VK_TRUE; }

  void Init() override;

  std::vector<core::vulkan::BindingInfo> GetBindingInfo() const override;
  const std::vector<uint32_t> LoadVertexShader() const override;
  const std::vector<uint32_t> LoadFragmentShader() const override;
  std::vector<VkVertexInputBindingDescription> GetVertexBindingDescriptions() const override;
  std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() const override;

 private:
  struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    float ui;  // 1 for view-space HUD geometry (crosshair), else 0
  };

  struct UniformBufferObject {
    glm::mat4 project;
    glm::mat4 viewmodel;
    float flash;
    float pad0;
    float pad1;
    float pad2;
  };

  void LoadModel(const std::string& model_path);
  void AppendCrosshair();
  void CreateTextureImage(const std::string& image_path);
  void CreateBuffers();

  std::vector<Vertex> vertices_;
  std::vector<uint32_t> indices_;

  core::vulkan::VulkanBuffer vertex_buffer_staging_;
  core::vulkan::VulkanBuffer vertex_buffer_local_;
  core::vulkan::VulkanBuffer index_buffer_staging_;
  core::vulkan::VulkanBuffer index_buffer_local_;
  core::vulkan::VulkanBuffer uniform_buffer_;

  core::vulkan::VulkanImage texture_image_;
  core::vulkan::VulkanSampler sampler_;
};

}  // namespace core
