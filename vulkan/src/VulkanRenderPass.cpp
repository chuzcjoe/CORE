#include "VulkanRenderPass.h"

namespace core {
namespace vulkan {

VulkanRenderPass::VulkanRenderPass(VulkanContext* context, VkFormat attachment_format)
    : context_(context) {
  // TODO: adapt to different rendering purposes
  VkAttachmentDescription attachment_description{};
  attachment_description.format = attachment_format;
  attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference attachment_ref{};
  attachment_ref.attachment = 0;
  attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &attachment_ref;
  subpass.pDepthStencilAttachment = nullptr;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &attachment_description;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  VkResult result =
      vkCreateRenderPass(context_->logical_device, &render_pass_info, nullptr, &render_pass_);
  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateRenderPass failed with result: " << result << std::endl;
    throw std::runtime_error("Failed to create render pass");
  }
  std::cout << "Render pass created successfully, handle: " << render_pass_ << std::endl;
}

VulkanRenderPass::~VulkanRenderPass() {
  vkDestroyRenderPass(context_->logical_device, render_pass_, nullptr);
}

}  // namespace vulkan
}  // namespace core