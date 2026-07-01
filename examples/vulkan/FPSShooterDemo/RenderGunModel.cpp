#include "RenderGunModel.h"

#include <cstring>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace core {

RenderGunModel::RenderGunModel(core::vulkan::VulkanContext* context,
                               const core::vulkan::DynamicRenderingInfo& dynamic_rendering_info)
    : core::vulkan::VulkanRender(context, dynamic_rendering_info), sampler_(context) {}

void RenderGunModel::Init(const std::string& image_path, const std::string& model_path) {
  CreateTextureImage(image_path);
  LoadModel(model_path);
  AppendCrosshair();
  CreateBuffers();
  Init();
}

void RenderGunModel::Init() {
  core::vulkan::VulkanRender::Init();

  CreateUniformBufferDescriptorSet(0, uniform_buffer_);
  CreateCombinedImageSamplerDescriptorSet(1, texture_image_.image_view, sampler_.sampler);
  vkUpdateDescriptorSets(context_->logical_device, writes_.size(), writes_.data(), 0, nullptr);

  vertex_buffer_staging_.MapData([this](void* data) {
    memcpy(data, vertices_.data(), sizeof(vertices_[0]) * vertices_.size());
  });
  index_buffer_staging_.MapData(
      [this](void* data) { memcpy(data, indices_.data(), sizeof(indices_[0]) * indices_.size()); });
  vertex_buffer_staging_.CopyToBuffer(vertex_buffer_local_);
  index_buffer_staging_.CopyToBuffer(index_buffer_local_);

  uniform_buffer_.MapData([](void* data) {
    UniformBufferObject init{glm::mat4(1.0f), glm::mat4(1.0f), 0.0f, 0.0f, 0.0f, 0.0f};
    memcpy(data, &init, sizeof(UniformBufferObject));
  });
}

void RenderGunModel::Render(VkCommandBuffer command_buffer, VkExtent2D extent) {
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
  vkCmdBindIndexBuffer(command_buffer, index_buffer_local_.buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                          &descriptor_set_, 0, nullptr);
  vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices_.size()), 1, 0, 0, 0);
}

void RenderGunModel::UpdateUniformBuffer(const glm::mat4& project, const glm::mat4& viewmodel,
                                         float flash) {
  uniform_buffer_.MapData([&](void* data) {
    UniformBufferObject ubo{project, viewmodel, flash, 0.0f, 0.0f, 0.0f};
    ubo.project[1][1] *= -1;  // Vulkan Y-flip
    memcpy(data, &ubo, sizeof(UniformBufferObject));
  });
}

std::vector<core::vulkan::BindingInfo> RenderGunModel::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
          {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}};
}

const std::vector<uint32_t> RenderGunModel::LoadVertexShader() const {
  static const std::vector<uint32_t> shader_code =
#include "FpsGunModel.vert.spv"
      ;
  return shader_code;
}

const std::vector<uint32_t> RenderGunModel::LoadFragmentShader() const {
  static const std::vector<uint32_t> shader_code =
#include "FpsGunModel.frag.spv"
      ;
  return shader_code;
}

std::vector<VkVertexInputBindingDescription> RenderGunModel::GetVertexBindingDescriptions() const {
  VkVertexInputBindingDescription b{};
  b.binding = 0;
  b.stride = sizeof(Vertex);
  b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return {b};
}

std::vector<VkVertexInputAttributeDescription> RenderGunModel::GetVertexAttributeDescriptions()
    const {
  std::vector<VkVertexInputAttributeDescription> a(4);
  a[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)};
  a[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)};
  a[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)};
  a[3] = {3, 0, VK_FORMAT_R32_SFLOAT, offsetof(Vertex, ui)};
  return a;
}

void RenderGunModel::LoadModel(const std::string& model_path) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn;
  std::string err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str())) {
    throw std::runtime_error("failed to load gun model: " + warn + err);
  }

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Vertex vertex{};
      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};
      if (index.normal_index >= 0) {
        vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                         attrib.normals[3 * index.normal_index + 1],
                         attrib.normals[3 * index.normal_index + 2]};
      } else {
        vertex.normal = {0.0f, 0.0f, 1.0f};
      }
      if (index.texcoord_index >= 0) {
        vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0],
                     1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
      } else {
        vertex.uv = {0.0f, 0.0f};
      }
      vertex.ui = 0.0f;

      indices_.push_back(static_cast<uint32_t>(vertices_.size()));
      vertices_.push_back(vertex);
    }
  }
}

void RenderGunModel::AppendCrosshair() {
  // Two thin white bars forming a plus, authored in view space at z = -0.5.
  const float kZ = -0.5f;
  const glm::vec3 n(0.0f, 0.0f, 1.0f);
  auto add_quad = [&](float hx, float hy) {
    const uint32_t base = static_cast<uint32_t>(vertices_.size());
    vertices_.push_back({{-hx, -hy, kZ}, n, {0.0f, 0.0f}, 1.0f});
    vertices_.push_back({{hx, -hy, kZ}, n, {0.0f, 0.0f}, 1.0f});
    vertices_.push_back({{hx, hy, kZ}, n, {0.0f, 0.0f}, 1.0f});
    vertices_.push_back({{-hx, hy, kZ}, n, {0.0f, 0.0f}, 1.0f});
    indices_.push_back(base + 0);
    indices_.push_back(base + 1);
    indices_.push_back(base + 2);
    indices_.push_back(base + 0);
    indices_.push_back(base + 2);
    indices_.push_back(base + 3);
  };
  add_quad(0.012f, 0.0018f);  // horizontal bar
  add_quad(0.0018f, 0.012f);  // vertical bar
}

void RenderGunModel::CreateTextureImage(const std::string& image_path) {
  int texture_width, texture_height, texture_channels;
  stbi_uc* pixels = stbi_load(image_path.c_str(), &texture_width, &texture_height,
                              &texture_channels, STBI_rgb_alpha);
  if (!pixels) {
    throw std::runtime_error("failed to load gun texture: " + image_path);
  }
  const VkDeviceSize image_size = static_cast<VkDeviceSize>(texture_width) * texture_height * 4;

  core::vulkan::VulkanBuffer staging_buffer(
      context_, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  staging_buffer.MapData(
      [&pixels, image_size](void* data) { memcpy(data, pixels, static_cast<size_t>(image_size)); });
  stbi_image_free(pixels);

  texture_image_ =
      core::vulkan::VulkanImage(context_, texture_width, texture_height, VK_FORMAT_R8G8B8A8_SRGB,
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                VK_IMAGE_ASPECT_COLOR_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  texture_image_.TransitionImageLayout(
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_FORMAT_R8G8B8A8_SRGB);
  staging_buffer.CopyToImage(texture_image_, static_cast<uint32_t>(texture_width),
                             static_cast<uint32_t>(texture_height));
  texture_image_.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                       VK_FORMAT_R8G8B8A8_SRGB);
}

void RenderGunModel::CreateBuffers() {
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
