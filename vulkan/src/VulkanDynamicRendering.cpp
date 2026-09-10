#include "VulkanDynamicRendering.h"

#include "VulkanUtils.h"

namespace core {
namespace vulkan {

VulkanDynamicRendering::VulkanDynamicRendering(VulkanContext* context) : context_(context) {
  LoadDynamicRenderingCommands();
}

void VulkanDynamicRendering::BeginDynamicRendering(VkCommandBuffer command_buffer,
                                                   VkImageView target_image_view, VkExtent2D extent,
                                                   VkClearValue clear_value) const {
  BeginDynamicRendering(command_buffer, target_image_view, VK_NULL_HANDLE, extent, clear_value);
}

void VulkanDynamicRendering::BeginDynamicRendering(VkCommandBuffer command_buffer,
                                                   VkImageView target_image_view, VkExtent2D extent,
                                                   VkClearValue clear_value,
                                                   VkImageView depth_image_view,
                                                   VkClearValue depth_clear_value) const {
  BeginDynamicRendering(command_buffer, target_image_view, VK_NULL_HANDLE, extent, clear_value,
                        depth_image_view, depth_clear_value);
}

void VulkanDynamicRendering::BeginDynamicRendering(VkCommandBuffer command_buffer,
                                                   VkImageView target_image_view,
                                                   VkImageView resolve_image_view,
                                                   VkExtent2D extent, VkClearValue clear_value,
                                                   VkImageView depth_image_view,
                                                   VkClearValue depth_clear_value) const {
  const bool has_resolve_attachment = resolve_image_view != VK_NULL_HANDLE;
  VkRenderingAttachmentInfo color_attachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = target_image_view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = has_resolve_attachment ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE,
      .resolveImageView = resolve_image_view,
      .resolveImageLayout = has_resolve_attachment ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                                   : VK_IMAGE_LAYOUT_UNDEFINED,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp =
          has_resolve_attachment ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = clear_value};
  VkRenderingAttachmentInfo depth_attachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = depth_image_view,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = depth_clear_value};
  VkRenderingInfo rendering_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {.offset = {0, 0}, .extent = extent},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment,
      .pDepthAttachment = depth_image_view == VK_NULL_HANDLE ? nullptr : &depth_attachment};

  vk_cmd_begin_rendering_(command_buffer, &rendering_info);
}

void VulkanDynamicRendering::EndDynamicRendering(VkCommandBuffer command_buffer) const {
  vk_cmd_end_rendering_(command_buffer);
}

void VulkanDynamicRendering::LoadDynamicRenderingCommands() {
  const auto commands = core::vulkan::LoadDynamicRenderingCommands(context_->logical_device);
  vk_cmd_begin_rendering_ = commands.vkCmdBeginRendering;
  vk_cmd_end_rendering_ = commands.vkCmdEndRendering;

  if (!vk_cmd_begin_rendering_ || !vk_cmd_end_rendering_) {
    throw std::runtime_error("failed to load dynamic rendering commands");
  }
}

}  // namespace vulkan
}  // namespace core
