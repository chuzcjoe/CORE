#include "VulkanRenderPass.h"

namespace core {
namespace vulkan {

Result<std::unique_ptr<VulkanRenderPass>> VulkanRenderPass::Create(
    VulkanContext* context, VkFormat color_attachment_format,
    const bool enable_depth_buffer) {
  std::unique_ptr<VulkanRenderPass> object(new VulkanRenderPass());
  const Status status =
      object->Init(context, color_attachment_format, enable_depth_buffer);
  if (!status.ok()) {
    return Result<std::unique_ptr<VulkanRenderPass>>(status);
  }
  return Result<std::unique_ptr<VulkanRenderPass>>(std::move(object));
}

Status VulkanRenderPass::Init(VulkanContext* context,
                              VkFormat color_attachment_format,
                              const bool enable_depth_buffer) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanRenderPass initialization is single-use");
  }
  initialized_ = true;
  context_ = context;
  if (context_ == nullptr || !context_->is_initialized()) {
    return Status(ErrorCode::kInvalidState, "A live Vulkan device is required");
  }

  has_color_attachment_ = color_attachment_format != VK_FORMAT_UNDEFINED;
  has_depth_attachment_ = enable_depth_buffer;

  // TODO: adapt to different rendering purposes

  if (enable_depth_buffer) {
    const auto depth_format_result =
        FindDepthFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                         VK_FORMAT_D24_UNORM_S8_UINT},
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    if (!depth_format_result.ok()) {
      return depth_format_result.status();
    }
    depth_format = *depth_format_result;
  }

  VkAttachmentDescription color_attachment_description{};
  if (has_color_attachment_) {
    color_attachment_description.format = color_attachment_format;
    color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  }

  VkAttachmentDescription depth_attachment_description{};
  if (has_depth_attachment_) {
    depth_attachment_description.format = depth_format;
    depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }

  VkAttachmentReference color_attachment_ref{};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref{};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = has_color_attachment_ ? 1 : 0;
  subpass.pColorAttachments = has_color_attachment_ ? &color_attachment_ref : nullptr;
  subpass.pDepthStencilAttachment = has_depth_attachment_ ? &depth_attachment_ref : nullptr;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  if (has_depth_attachment_) {
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  } else {
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  }

  std::vector<VkAttachmentDescription> attachments;
  if (has_color_attachment_) {
    attachments.emplace_back(color_attachment_description);
  }
  if (has_depth_attachment_) {
    attachments.emplace_back(depth_attachment_description);
  }

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
  render_pass_info.pAttachments = attachments.data();
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  VkResult result =
      vkCreateRenderPass(context_->logical_device, &render_pass_info, nullptr, &render_pass_);
  if (result != VK_SUCCESS) {
    return Status(ErrorCode::kBackend, "Failed to create render pass", "Vulkan",
                  result);
  }

  return Status();
}

VulkanRenderPass::~VulkanRenderPass() {
  if (context_ == nullptr || context_->logical_device == VK_NULL_HANDLE) {
    return;
  }

  vkDestroyRenderPass(context_->logical_device, render_pass_, nullptr);
}

Result<VkFormat> VulkanRenderPass::FindDepthFormat(
    const std::vector<VkFormat>& candidates, VkImageTiling tiling,
    VkFormatFeatureFlags features) const {
  if (context_ == nullptr) {
    return Result<VkFormat>(
        Status(ErrorCode::kInvalidState,
               "VulkanRenderPass requires successful initialization"));
  }

  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(context_->physical_device, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return Result<VkFormat>(format);
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return Result<VkFormat>(format);
    }
  }
  return Result<VkFormat>(
      Status(ErrorCode::kUnsupported, "failed to find supported format!"));
}

}  // namespace vulkan
}  // namespace core
