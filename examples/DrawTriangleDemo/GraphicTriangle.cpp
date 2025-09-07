#include "GraphicTriangle.h"

namespace core {

GraphicTriangle::GraphicTriangle(core::vulkan::VulkanContext* context,
                                 core::vulkan::VulkanRenderPass render_pass)
    : core::vulkan::VulkanGraphic(context, render_pass) {
  CreateVertexBuffer();
}

void GraphicTriangle::Init() { core::vulkan::VulkanGraphic::Init(); }

void GraphicTriangle::Render(VkCommandBuffer command_buffer) {
  const VkBuffer vertex_buffers[] = {vertex_buffer_.buffer};
  const VkDeviceSize offsets[] = {0};
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                          &descriptor_set_, 0, nullptr);
  vkCmdDraw(command_buffer, static_cast<uint32_t>(vertices_.size()), 1, 0, 0);
}

std::vector<core::vulkan::BindingInfo> GraphicTriangle::GetBindingInfo() const {
  // Drawing a triangle does not need to bind resources
  return {};
}

const std::vector<uint32_t> GraphicTriangle::LoadVertexShader() const {
  static const std::vector<uint32_t> shader_code =
#include "Triangle.vert.spv"
      ;
  return shader_code;
}

const std::vector<uint32_t> GraphicTriangle::LoadFragmentShader() const {
  static const std::vector<uint32_t> shader_code =
#include "Triangle.frag.spv"
      ;
  return shader_code;
}

std::vector<VkVertexInputBindingDescription> GraphicTriangle::GetVertexBindingDescriptions() const {
  return {};
}

std::vector<VkVertexInputAttributeDescription> GraphicTriangle::GetVertexAttributeDescriptions()
    const {
  return {};
}

void GraphicTriangle::CreateVertexBuffer() {
  const VkDeviceSize size = sizeof(core::vulkan::Vertex) * sizeof(vertices_.size());
  vertex_buffer_ = core::vulkan::VulkanBuffer(
      context_, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  vertex_buffer_.MapData([this](void* data) { memcpy(data, vertices_.data(), size); });
}

}  // namespace core