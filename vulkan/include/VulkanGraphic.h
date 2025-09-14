#pragma once

#include <vector>

#include "VulkanBase.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanRenderPass.h"
#include "glm/glm.hpp"

namespace core {
namespace vulkan {

class VulkanGraphic : public VulkanBase {
 public:
  VulkanGraphic(VulkanContext* context, VulkanRenderPass& render_pass);

 protected:
  void CreatePipeline() override;

 private:
  // Derived class needs to configure these explicitly
  virtual const std::vector<uint32_t> LoadVertexShader() const = 0;
  virtual const std::vector<uint32_t> LoadFragmentShader() const = 0;
  virtual std::array<VkVertexInputBindingDescription, 1> GetVertexBindingDescriptions() const = 0;
  virtual std::array<VkVertexInputAttributeDescription, 2> GetVertexAttributeDescriptions()
      const = 0;

  VulkanRenderPass& render_pass_;
};

}  // namespace vulkan
}  // namespace core