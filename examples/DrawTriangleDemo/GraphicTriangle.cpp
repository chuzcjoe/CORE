#include "GraphicTriangle.h"

namespace core {

GraphicTriangle::GraphicTriangle(core::vulkan::VulkanContext* context,
                                 core::vulkan::VulkanRenderPass& render_pass)
    : core::vulkan::VulkanGraphic(context, render_pass) {
  CreateVertexBuffer();
}

void GraphicTriangle::Init() {
  core::vulkan::VulkanGraphic::Init();
  vertex_buffer_.MapData([this](void* data) {
    memcpy(data, vertices_.data(), sizeof(vertices_[0]) * vertices_.size());
  });
}

void GraphicTriangle::Render(VkCommandBuffer command_buffer, VkExtent2D extent) {
  const VkBuffer vertex_buffers[] = {vertex_buffer_.buffer};
  const VkDeviceSize offsets[] = {0};
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)extent.width;
  viewport.height = (float)extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);

  vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
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

std::array<VkVertexInputBindingDescription, 1> GraphicTriangle::GetVertexBindingDescriptions()
    const {
  VkVertexInputBindingDescription binding_description{};
  binding_description.binding = 0;
  binding_description.stride = sizeof(core::vulkan::Vertex);
  binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return {binding_description};
}

std::array<VkVertexInputAttributeDescription, 2> GraphicTriangle::GetVertexAttributeDescriptions()
    const {
  std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{};
  attribute_descriptions[0].binding = 0;
  attribute_descriptions[0].location = 0;
  attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
  attribute_descriptions[0].offset = offsetof(core::vulkan::Vertex, pos);

  attribute_descriptions[1].binding = 0;
  attribute_descriptions[1].location = 1;
  attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attribute_descriptions[1].offset = offsetof(core::vulkan::Vertex, color);
  return attribute_descriptions;
}

void GraphicTriangle::CreateVertexBuffer() {
  const VkDeviceSize size = sizeof(vertices_[0]) * vertices_.size();

  vertex_buffer_ = core::vulkan::VulkanBuffer(
      context_, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

}  // namespace core