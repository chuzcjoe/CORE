#pragma once

#include <vector>

#include "Bitmap.h"
#include "VulkanGraphic.h"
#include "VulkanImage.h"
#include "VulkanRenderPass.h"
#include "VulkanSampler.h"
#include "VulkanUtils.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/hash.hpp"

namespace core {

class GraphicCubeMap : public core::vulkan::VulkanGraphic {
 public:
  GraphicCubeMap(core::vulkan::VulkanContext* context,
                 const core::vulkan::DynamicRenderingInfo& dynamic_rendering_info);

  void Init() override;
  void Render(VkCommandBuffer command_buffer, VkExtent2D extent);

  void UpdateUniformBuffer(const int width, const int height, const glm::mat4& view_matrix,
                           const float rotation);

 protected:
  VkCullModeFlags SetCullMode() const override { return VK_CULL_MODE_BACK_BIT; }
  VkFrontFace SetFrontFace() const override { return VK_FRONT_FACE_COUNTER_CLOCKWISE; }
  VkBool32 SetDepthTesting() const override { return VK_TRUE; }
  VkBool32 SetDepthWriting() const override { return VK_TRUE; }

  std::vector<core::vulkan::BindingInfo> GetBindingInfo() const override;
  const std::vector<uint32_t> LoadVertexShader() const override;
  const std::vector<uint32_t> LoadFragmentShader() const override;
  std::vector<VkVertexInputBindingDescription> GetVertexBindingDescriptions() const override;
  std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() const override;

 private:
  void CreateTextureImage(const std::string& image_path);

  struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 project;
  } uniform_data_;

  core::vulkan::VulkanBuffer vertex_buffer_staging_;
  core::vulkan::VulkanBuffer index_buffer_staging_;
  core::vulkan::VulkanBuffer vertex_buffer_local_;
  core::vulkan::VulkanBuffer index_buffer_local_;
  core::vulkan::VulkanBuffer uniform_buffer_;
  core::vulkan::VulkanImage cube_map_image_;
  core::vulkan::VulkanSampler sampler_;
};

}  // namespace core