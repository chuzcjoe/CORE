#include "GraphicTriangle.h"

namespace core {

GraphicTriangle::GraphicTriangle(core::vulkan::VulkanContext* context,
                                 core::vulkan::VulkanRenderPass& render_pass)
    : core::vulkan::VulkanGraphic(context, render_pass) {
  CreateVertexBuffer();
}

void GraphicTriangle::Init() {
  core::vulkan::VulkanGraphic::Init();

  CreateUniformBufferDescriptorSet(0, uniform_buffer_);
  vkUpdateDescriptorSets(context_->logical_device, writes_.size(), writes_.data(), 0, nullptr);

  vertex_buffer_staging_.MapData([this](void* data) {
    memcpy(data, vertices_.data(), sizeof(vertices_[0]) * vertices_.size());
  });
  index_buffer_staging_.MapData(
      [this](void* data) { memcpy(data, indices_.data(), sizeof(indices_[0]) * indices_.size()); });

  uniform_buffer_.MapData([this](void* data) {
    uniform_data_.model = glm::mat4(1.0f);
    uniform_data_.view = glm::mat4(1.0f);
    uniform_data_.project = glm::mat4(1.0f);
    memcpy(data, &uniform_data_, sizeof(UniformBufferObject));
  });

  vertex_buffer_staging_.CopyToBuffer(vertex_buffer_local_);
  index_buffer_staging_.CopyToBuffer(index_buffer_local_);
}

void GraphicTriangle::Render(VkCommandBuffer command_buffer, VkExtent2D extent) {
  const VkBuffer vertex_buffers[] = {vertex_buffer_local_.buffer};
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
  vkCmdBindIndexBuffer(command_buffer, index_buffer_local_.buffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                          &descriptor_set_, 0, nullptr);
  vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices_.size()), 1, 0, 0, 0);
}

std::vector<core::vulkan::BindingInfo> GraphicTriangle::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}};
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
  binding_description.stride = sizeof(Vertex);
  binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return {binding_description};
}

std::array<VkVertexInputAttributeDescription, 2> GraphicTriangle::GetVertexAttributeDescriptions()
    const {
  std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{};
  attribute_descriptions[0].binding = 0;
  attribute_descriptions[0].location = 0;
  attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
  attribute_descriptions[0].offset = offsetof(Vertex, pos);

  attribute_descriptions[1].binding = 0;
  attribute_descriptions[1].location = 1;
  attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attribute_descriptions[1].offset = offsetof(Vertex, color);
  return attribute_descriptions;
}

void GraphicTriangle::CreateVertexBuffer() {
  const VkDeviceSize vertex_buffer_size = sizeof(vertices_[0]) * vertices_.size();
  const VkDeviceSize index_buffer_size = sizeof(indices_[0]) * indices_.size();

  vertex_buffer_staging_ = core::vulkan::VulkanBuffer(
      context_, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  vertex_buffer_local_ = core::vulkan::VulkanBuffer(
      context_, vertex_buffer_size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  index_buffer_staging_ = core::vulkan::VulkanBuffer(
      context_, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  index_buffer_local_ = core::vulkan::VulkanBuffer(
      context_, index_buffer_size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  uniform_buffer_ = core::vulkan::VulkanBuffer(
      context_, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void GraphicTriangle::UpdateUniformBuffer(const int width, const int height) {
  // printf("width: %d, height: %d\n", width, height);
  auto current_time = std::chrono::high_resolution_clock::now();
  float time =
      std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time_)
          .count();

  // TODO: maintain a persistent mapping pointer to avoid mapping every time
  uniform_buffer_.MapData([this, time, width, height](void* data) {
    uniform_data_.model =
        glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    uniform_data_.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 0.0f, 1.0f));
    uniform_data_.project =
        glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 10.0f);
    uniform_data_.project[1][1] *= -1;  // Invert Y for Vulkan

    memcpy(data, &uniform_data_, sizeof(UniformBufferObject));
  });
}

}  // namespace core