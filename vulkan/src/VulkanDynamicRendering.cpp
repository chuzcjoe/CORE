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
  BeginDynamicRendering(command_buffer, target_image_view, extent, clear_value, VK_NULL_HANDLE, {});
}

void VulkanDynamicRendering::BeginDynamicRendering(VkCommandBuffer command_buffer,
                                                   VkImageView target_image_view, VkExtent2D extent,
                                                   VkClearValue clear_value,
                                                   VkImageView depth_image_view,
                                                   VkClearValue depth_clear_value) const {
  VkRenderingAttachmentInfo color_attachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = target_image_view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
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
