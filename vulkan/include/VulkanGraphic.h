#pragma once

#include <vector>

#include "VulkanBase.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanRenderPass.h"

namespace core {
namespace vulkan {

class VulkanGraphic : public VulkanBase {
 public:
  VulkanGraphic(VulkanContext* context, VulkanRenderPass* render_pass);

  // Dynamic rendering
  VulkanGraphic(VulkanContext* context);

 protected:
  void CreatePipeline() override;

 private:
  // Derived class can customize these settings
  virtual VkCullModeFlags SetCullMode() const { return VK_CULL_MODE_BACK_BIT; }
  virtual VkFrontFace SetFrontFace() const { return VK_FRONT_FACE_CLOCKWISE; }
  virtual VkBool32 SetDepthTesting() const { return VK_FALSE; }
  virtual VkBool32 SetDepthWriting() const { return VK_FALSE; }

  // Derived class needs to configure these explicitly
  virtual const std::vector<uint32_t> LoadVertexShader() const = 0;
  virtual const std::vector<uint32_t> LoadFragmentShader() const = 0;
  virtual std::vector<VkVertexInputBindingDescription> GetVertexBindingDescriptions() const = 0;
  virtual std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() const = 0;

  VulkanRenderPass* render_pass_ = nullptr;
};

}  // namespace vulkan
}  // namespace core