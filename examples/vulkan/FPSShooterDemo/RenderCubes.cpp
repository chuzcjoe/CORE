#include "RenderCubes.h"

#include <algorithm>
#include <cstring>

namespace core {

RenderCubes::RenderCubes(core::vulkan::VulkanContext* context,
                         const core::vulkan::DynamicRenderingInfo& dynamic_rendering_info,
                         uint32_t max_instances)
    : core::vulkan::VulkanRender(context, dynamic_rendering_info), max_instances_(max_instances) {
  BuildGeometry();
  CreateBuffers();
}

void RenderCubes::Init() {
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

void RenderCubes::Render(VkCommandBuffer command_buffer, VkExtent2D extent) {
  if (instance_count_ == 0) return;

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

  const VkBuffer vertex_buffers[] = {vertex_buffer_local_.buffer, instance_buffer_.buffer};
  const VkDeviceSize offsets[] = {0, 0};
  vkCmdBindVertexBuffers(command_buffer, 0, 2, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(command_buffer, index_buffer_local_.buffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                          &descriptor_set_, 0, nullptr);
  vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices_.size()), instance_count_, 0, 0,
                   0);
}

void RenderCubes::UpdateUniformBuffer(const glm::mat4& view, const glm::mat4& project) {
  uniform_buffer_.MapData([&](void* data) {
    UniformBufferObject ubo{view, project};
    ubo.project[1][1] *= -1;  // Vulkan Y-flip
    memcpy(data, &ubo, sizeof(UniformBufferObject));
  });
}

void RenderCubes::SetInstances(const std::vector<CubeInstance>& instances) {
  instance_count_ = std::min(static_cast<uint32_t>(instances.size()), max_instances_);
  if (instance_count_ == 0) return;
  instance_buffer_.MapData(
      [&](void* data) { memcpy(data, instances.data(), sizeof(CubeInstance) * instance_count_); });
}

std::vector<core::vulkan::BindingInfo> RenderCubes::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}};
}

const std::vector<uint32_t> RenderCubes::LoadVertexShader() const {
  static const std::vector<uint32_t> shader_code =
#include "FpsCube.vert.spv"
      ;
  return shader_code;
}

const std::vector<uint32_t> RenderCubes::LoadFragmentShader() const {
  static const std::vector<uint32_t> shader_code =
#include "FpsCube.frag.spv"
      ;
  return shader_code;
}

std::vector<VkVertexInputBindingDescription> RenderCubes::GetVertexBindingDescriptions() const {
  VkVertexInputBindingDescription vertex_binding{};
  vertex_binding.binding = 0;
  vertex_binding.stride = sizeof(Vertex);
  vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputBindingDescription instance_binding{};
  instance_binding.binding = 1;
  instance_binding.stride = sizeof(CubeInstance);
  instance_binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

  return {vertex_binding, instance_binding};
}

std::vector<VkVertexInputAttributeDescription> RenderCubes::GetVertexAttributeDescriptions() const {
  std::vector<VkVertexInputAttributeDescription> a(7);
  // Per-vertex.
  a[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)};
  a[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)};
  // Per-instance mat4 (one location per column).
  a[2] = {2, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(CubeInstance, model) + 0};
  a[3] = {3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(CubeInstance, model) + 16};
  a[4] = {4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(CubeInstance, model) + 32};
  a[5] = {5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(CubeInstance, model) + 48};
  // Per-instance color.
  a[6] = {6, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CubeInstance, color)};
  return a;
}

void RenderCubes::BuildGeometry() {
  const float h = 0.5f;
  const glm::vec3 normals[6] = {
      {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1},
  };
  // For each face, four corners in CCW order when viewed along +normal.
  const glm::vec3 face_corners[6][4] = {
      {{h, -h, -h}, {h, h, -h}, {h, h, h}, {h, -h, h}},      // +X
      {{-h, -h, h}, {-h, h, h}, {-h, h, -h}, {-h, -h, -h}},  // -X
      {{-h, h, -h}, {-h, h, h}, {h, h, h}, {h, h, -h}},      // +Y
      {{-h, -h, h}, {-h, -h, -h}, {h, -h, -h}, {h, -h, h}},  // -Y
      {{-h, -h, h}, {h, -h, h}, {h, h, h}, {-h, h, h}},      // +Z
      {{h, -h, -h}, {-h, -h, -h}, {-h, h, -h}, {h, h, -h}},  // -Z
  };

  vertices_.reserve(24);
  indices_.reserve(36);
  for (int f = 0; f < 6; ++f) {
    const uint16_t base = static_cast<uint16_t>(vertices_.size());
    for (int c = 0; c < 4; ++c) {
      vertices_.push_back({face_corners[f][c], normals[f]});
    }
    indices_.push_back(base + 0);
    indices_.push_back(base + 1);
    indices_.push_back(base + 2);
    indices_.push_back(base + 0);
    indices_.push_back(base + 2);
    indices_.push_back(base + 3);
  }
}

void RenderCubes::CreateBuffers() {
  const VkDeviceSize v_size = sizeof(vertices_[0]) * vertices_.size();
  const VkDeviceSize i_size = sizeof(indices_[0]) * indices_.size();
  const VkDeviceSize inst_size = sizeof(CubeInstance) * max_instances_;

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

  instance_buffer_ = core::vulkan::VulkanBuffer(
      context_, inst_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  uniform_buffer_ = core::vulkan::VulkanBuffer(
      context_, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

}  // namespace core
