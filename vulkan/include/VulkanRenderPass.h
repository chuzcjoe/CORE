#pragma once

#include <vector>

#include "VulkanBase.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "glm/glm.hpp"

namespace core {
namespace vulkan {

class VulkanRenderPass {
 public:
  VulkanRenderPass(VulkanContext* context, VkFormat attachment_format);
  ~VulkanRenderPass();

  VkRenderPass GetRenderPass() const { return render_pass_; }

 private:
  VkRenderPass render_pass_;
  VulkanContext* context_;
};

}  // namespace vulkan
}  // namespace core