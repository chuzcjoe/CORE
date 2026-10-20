#pragma once

#include <memory>
#include <vector>

#include "Status.h"
#include "VulkanBase.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "glm/glm.hpp"

namespace core {
namespace vulkan {

class VulkanRenderPass {
 public:
  VulkanRenderPass(const VulkanRenderPass&) = delete;
  VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;
  VulkanRenderPass() = default;

  static Result<std::unique_ptr<VulkanRenderPass>> Create(
      VulkanContext* context,
      VkFormat color_attachment_format = VK_FORMAT_UNDEFINED,
      const bool enable_depth_buffer = false);
  Status Init(VulkanContext* context,
              VkFormat color_attachment_format = VK_FORMAT_UNDEFINED,
              const bool enable_depth_buffer = false);
  ~VulkanRenderPass();

  VkRenderPass GetRenderPass() const { return render_pass_; }

  VkFormat depth_format = VK_FORMAT_UNDEFINED;

 private:
  Result<VkFormat> FindDepthFormat(const std::vector<VkFormat>& candidates,
                                   VkImageTiling tiling,
                                   VkFormatFeatureFlags features) const;

  VulkanContext* context_ = nullptr;
  VkRenderPass render_pass_ = VK_NULL_HANDLE;

  bool has_color_attachment_ = false;
  bool has_depth_attachment_ = false;

 private:
  bool initialized_ = false;
};

}  // namespace vulkan
}  // namespace core
