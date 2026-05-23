#include "RenderGround.h"

#include <cstring>

namespace core {

RenderGround::RenderGround(core::vulkan::VulkanContext* context,
                           const core::vulkan::DynamicRenderingInfo& dynamic_rendering_info)
    : core::vulkan::VulkanRender(context, dynamic_rendering_info) {
  CreateBuffers();
}

void RenderGround::Init() {
  core::vulkan::VulkanRender::Init();

  CreateUniformBufferDescriptorSet(0, uniform_buffer_);
  vkUpdateDescriptorSets(context_->logical_device, writes_.size(), writes_.data(), 0, nullptr);

  vertex_buffer_staging_.MapData([this](void* data) {
    memcpy(data, vertices_.data(), sizeof(vertices_[0]) * vertices_.size());
  });
  index_buffer_staging_.MapData(
      [this](void* data) { memcpy(data, indices_.data(), sizeof(indices_[0]) * indices_.size()); });
  vertex_buffer_staging_.CopyToBuffer(vertex_buffer_local_);
  index_buffer_staging_.CopyToBuffer(index_buffer_local_);

  uniform_buffer_.MapData([](void* data) {
    UniformBufferObject init{glm::mat4(1.0f), glm::mat4(1.0f)};
    memcpy(data, &init, sizeof(UniformBufferObject));
  });
}

void RenderGround::Render(VkCommandBuffer command_buffer, VkExtent2D extent) {
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);

  const VkBuffer vertex_buffers[] = {vertex_buffer_local_.buffer};
  const VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(command_buffer, index_buffer_local_.buffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                          &descriptor_set_, 0, nullptr);
  vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices_.size()), 1, 0, 0, 0);
}

void RenderGround::UpdateUniformBuffer(const glm::mat4& view, const glm::mat4& project) {
  uniform_buffer_.MapData([&](void* data) {
    UniformBufferObject ubo{view, project};
    ubo.project[1][1] *= -1;  // Vulkan Y-flip
    memcpy(data, &ubo, sizeof(UniformBufferObject));
  });
}

std::vector<core::vulkan::BindingInfo> RenderGround::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}};
}

const std::vector<uint32_t> RenderGround::LoadVertexShader() const {
  static const std::vector<uint32_t> shader_code =
#include "Ground.vert.spv"
      ;
  return shader_code;
}

const std::vector<uint32_t> RenderGround::LoadFragmentShader() const {
  static const std::vector<uint32_t> shader_code =
#include "Ground.frag.spv"
      ;
  return shader_code;
}

std::vector<VkVertexInputBindingDescription> RenderGround::GetVertexBindingDescriptions() const {
  VkVertexInputBindingDescription b{};
  b.binding = 0;
  b.stride = sizeof(glm::vec3);
  b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return {b};
}

std::vector<VkVertexInputAttributeDescription> RenderGround::GetVertexAttributeDescriptions()
    const {
  VkVertexInputAttributeDescription a{};
  a.binding = 0;
  a.location = 0;
  a.format = VK_FORMAT_R32G32B32_SFLOAT;
  a.offset = 0;
  return {a};
}

void RenderGround::CreateBuffers() {
  const VkDeviceSize v_size = sizeof(vertices_[0]) * vertices_.size();
  const VkDeviceSize i_size = sizeof(indices_[0]) * indices_.size();

  vertex_buffer_staging_ = core::vulkan::VulkanBuffer(
      context_, v_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  vertex_buffer_local_ = core::vulkan::VulkanBuffer(
      context_, v_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  index_buffer_staging_ = core::vulkan::VulkanBuffer(
      context_, i_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  index_buffer_local_ = core::vulkan::VulkanBuffer(
      context_, i_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  uniform_buffer_ = core::vulkan::VulkanBuffer(
      context_, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

}  // namespace core
