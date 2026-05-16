#include "GraphicGalaxy.h"

#include <cmath>
#include <cstring>
#include <random>

namespace core {

GraphicGalaxy::GraphicGalaxy(core::vulkan::VulkanContext* context,
                             const core::vulkan::DynamicRenderingInfo& dynamic_rendering_info,
                             uint32_t star_count)
    : core::vulkan::VulkanGraphic(context, dynamic_rendering_info), star_count_(star_count) {
  GenerateStars();
  CreateBuffers();
}

void GraphicGalaxy::Init() {
  core::vulkan::VulkanGraphic::Init();

  CreateUniformBufferDescriptorSet(0, uniform_buffer_);
  vkUpdateDescriptorSets(context_->logical_device, writes_.size(), writes_.data(), 0, nullptr);

  quad_buffer_staging_.MapData([this](void* data) {
    memcpy(data, quad_corners_.data(), sizeof(quad_corners_[0]) * quad_corners_.size());
  });
  index_buffer_staging_.MapData([this](void* data) {
    memcpy(data, quad_indices_.data(), sizeof(quad_indices_[0]) * quad_indices_.size());
  });
  instance_buffer_staging_.MapData([this](void* data) {
    memcpy(data, instances_.data(), sizeof(InstanceData) * instances_.size());
  });

  quad_buffer_staging_.CopyToBuffer(quad_buffer_local_);
  index_buffer_staging_.CopyToBuffer(index_buffer_local_);
  instance_buffer_staging_.CopyToBuffer(instance_buffer_local_);

  uniform_buffer_.MapData([this](void* data) {
    uniform_data_.model = glm::mat4(1.0f);
    uniform_data_.view = glm::mat4(1.0f);
    uniform_data_.project = glm::mat4(1.0f);
    uniform_data_.time = 0.0f;
    uniform_data_.aspect = 1.0f;
    memcpy(data, &uniform_data_, sizeof(UniformBufferObject));
  });
}

void GraphicGalaxy::Render(VkCommandBuffer command_buffer, VkExtent2D extent) {
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

  const VkBuffer vertex_buffers[] = {quad_buffer_local_.buffer, instance_buffer_local_.buffer};
  const VkDeviceSize offsets[] = {0, 0};
  vkCmdBindVertexBuffers(command_buffer, 0, 2, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(command_buffer, index_buffer_local_.buffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                          &descriptor_set_, 0, nullptr);
  vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(quad_indices_.size()), star_count_, 0, 0,
                   0);
}

VkPipelineColorBlendAttachmentState GraphicGalaxy::SetColorBlendAttachment() const {
  // Additive blending so stars accumulate glow
  VkPipelineColorBlendAttachmentState s{};
  s.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  s.blendEnable = VK_TRUE;
  s.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  s.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
  s.colorBlendOp = VK_BLEND_OP_ADD;
  s.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  s.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  s.alphaBlendOp = VK_BLEND_OP_ADD;
  return s;
}

std::vector<core::vulkan::BindingInfo> GraphicGalaxy::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}};
}

const std::vector<uint32_t> GraphicGalaxy::LoadVertexShader() const {
  static const std::vector<uint32_t> shader_code =
#include "Galaxy.vert.spv"
      ;
  return shader_code;
}

const std::vector<uint32_t> GraphicGalaxy::LoadFragmentShader() const {
  static const std::vector<uint32_t> shader_code =
#include "Galaxy.frag.spv"
      ;
  return shader_code;
}

std::vector<VkVertexInputBindingDescription> GraphicGalaxy::GetVertexBindingDescriptions() const {
  VkVertexInputBindingDescription quad_binding{};
  quad_binding.binding = 0;
  quad_binding.stride = sizeof(glm::vec2);
  quad_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputBindingDescription instance_binding{};
  instance_binding.binding = 1;
  instance_binding.stride = sizeof(InstanceData);
  instance_binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

  return {quad_binding, instance_binding};
}

std::vector<VkVertexInputAttributeDescription> GraphicGalaxy::GetVertexAttributeDescriptions()
    const {
  std::vector<VkVertexInputAttributeDescription> attrs(4);
  attrs[0].binding = 0;
  attrs[0].location = 0;
  attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
  attrs[0].offset = 0;

  attrs[1].binding = 1;
  attrs[1].location = 1;
  attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrs[1].offset = offsetof(InstanceData, pos);

  attrs[2].binding = 1;
  attrs[2].location = 2;
  attrs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrs[2].offset = offsetof(InstanceData, color);

  attrs[3].binding = 1;
  attrs[3].location = 3;
  attrs[3].format = VK_FORMAT_R32_SFLOAT;
  attrs[3].offset = offsetof(InstanceData, size);
  return attrs;
}

void GraphicGalaxy::GenerateStars() {
  instances_.reserve(star_count_);

  std::mt19937 rng(42);
  std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
  std::normal_distribution<float> gauss(0.0f, 1.0f);

  // Star color palette — bluish for young arm stars, yellowish for general disk,
  // reddish/orange for bulge.
  const glm::vec3 kBlue = glm::vec3(0.65f, 0.80f, 1.0f);
  const glm::vec3 kWhite = glm::vec3(1.0f, 0.97f, 0.88f);
  const glm::vec3 kOrange = glm::vec3(1.0f, 0.78f, 0.50f);
  const glm::vec3 kRed = glm::vec3(1.0f, 0.55f, 0.35f);

  const uint32_t bulge_count = star_count_ / 4;
  const uint32_t disk_count = star_count_ - bulge_count;

  // Disk stars — exponential radial profile with two spiral arms
  const float arm_count = 2.0f;
  const float arm_tightness = 0.55f;     // larger => tighter wind
  const float arm_spread = 0.35f;        // angular jitter from arm center (radians)
  const float disk_scale_radius = 1.6f;  // exponential scale length
  const float disk_thickness = 0.05f;    // vertical scale height
  const float max_radius = 5.0f;

  for (uint32_t i = 0; i < disk_count; ++i) {
    // Sample radius with exponential falloff
    float r;
    do {
      r = -disk_scale_radius * std::log(std::max(uniform(rng), 1e-4f));
    } while (r > max_radius || r < 0.15f);

    // Pick an arm and add jitter
    float arm_idx = std::floor(uniform(rng) * arm_count);
    float arm_phase = arm_idx * (2.0f * M_PI / arm_count);
    float base_angle = arm_tightness * std::log(r + 0.01f) + arm_phase;
    float jitter = gauss(rng) * arm_spread * (0.5f + 0.5f * std::exp(-r * 0.3f));
    float theta = base_angle + jitter;

    float x = r * std::cos(theta);
    float y = r * std::sin(theta);
    float z = gauss(rng) * disk_thickness * (1.0f + 0.5f * std::exp(-r));

    InstanceData s{};
    s.pos = glm::vec3(x, y, z);

    // Color: arm stars (close to arm center) are bluer; bulk disk yellowish
    float arm_distance = std::abs(jitter) / arm_spread;
    float blueness = std::exp(-arm_distance * arm_distance * 2.0f);
    glm::vec3 c = glm::mix(kWhite, kBlue, blueness * 0.85f);

    // Outer disk slightly redder
    c = glm::mix(c, kOrange, glm::clamp(r / max_radius * 0.4f, 0.0f, 0.4f));

    // Per-star brightness variation
    float bright = 0.6f + 0.4f * uniform(rng);
    s.color = c * bright;

    // Size: a few brighter outliers, most small
    float size_roll = uniform(rng);
    s.size =
        (size_roll < 0.985f) ? (0.008f + 0.005f * uniform(rng)) : (0.020f + 0.020f * uniform(rng));
    instances_.emplace_back(s);
  }

  // Bulge stars — spherical-ish, denser at center
  for (uint32_t i = 0; i < bulge_count; ++i) {
    float r;
    do {
      r = std::abs(gauss(rng)) * 0.45f;
    } while (r > 1.2f);

    float theta = uniform(rng) * 2.0f * static_cast<float>(M_PI);
    float phi = std::acos(2.0f * uniform(rng) - 1.0f);

    float x = r * std::sin(phi) * std::cos(theta);
    float y = r * std::sin(phi) * std::sin(theta);
    float z = r * std::cos(phi) * 0.6f;  // flatten slightly toward disk plane

    InstanceData s{};
    s.pos = glm::vec3(x, y, z);
    glm::vec3 c = glm::mix(kOrange, kRed, uniform(rng) * 0.5f);
    float bright = 0.7f + 0.6f * uniform(rng);
    s.color = c * bright;
    float size_roll = uniform(rng);
    s.size =
        (size_roll < 0.95f) ? (0.010f + 0.006f * uniform(rng)) : (0.022f + 0.022f * uniform(rng));
    instances_.emplace_back(s);
  }
}

void GraphicGalaxy::CreateBuffers() {
  const VkDeviceSize quad_size = sizeof(quad_corners_[0]) * quad_corners_.size();
  const VkDeviceSize idx_size = sizeof(quad_indices_[0]) * quad_indices_.size();
  const VkDeviceSize inst_size = sizeof(InstanceData) * instances_.size();

  quad_buffer_staging_ = core::vulkan::VulkanBuffer(
      context_, quad_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  quad_buffer_local_ = core::vulkan::VulkanBuffer(
      context_, quad_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  index_buffer_staging_ = core::vulkan::VulkanBuffer(
      context_, idx_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  index_buffer_local_ = core::vulkan::VulkanBuffer(
      context_, idx_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  instance_buffer_staging_ = core::vulkan::VulkanBuffer(
      context_, inst_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  instance_buffer_local_ = core::vulkan::VulkanBuffer(
      context_, inst_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  uniform_buffer_ = core::vulkan::VulkanBuffer(
      context_, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void GraphicGalaxy::UpdateUniformBuffer(uint32_t width, uint32_t height) {
  auto current_time = std::chrono::high_resolution_clock::now();
  const float time =
      std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time_)
          .count();

  uniform_buffer_.MapData([this, time, width, height](void* data) {
    // Tilt the galaxy so we see it edge-on-ish
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(-65.0f), glm::vec3(1, 0, 0));
    uniform_data_.model = model;

    uniform_data_.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 7.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
    uniform_data_.project = glm::perspective(
        glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
    uniform_data_.project[1][1] *= -1;  // Vulkan Y-flip
    uniform_data_.time = time;
    uniform_data_.aspect = static_cast<float>(width) / static_cast<float>(height);
    memcpy(data, &uniform_data_, sizeof(UniformBufferObject));
  });
}

}  // namespace core
