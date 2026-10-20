#include "RenderSkybox.h"

#include <cstring>
#include <vector>

#include "Status.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace core {

RenderSkybox::RenderSkybox(
    core::vulkan::VulkanContext* context,
    const core::vulkan::DynamicRenderingInfo& dynamic_rendering_info)
    : core::vulkan::VulkanRender(context, dynamic_rendering_info) {}

core::Status RenderSkybox::Init() {
  {
    const core::Status status = sampler_.Init(context_);
    if (!status.ok()) {
      return status;
    }
  }

  {
    const core::Status status = core::vulkan::VulkanRender::Init();
    if (!status.ok()) {
      return status;
    }
  }

  CreateUniformBufferDescriptorSet(0, uniform_buffer_);
  CreateCombinedImageSamplerDescriptorSet(1, cube_map_image_.image_view, sampler_.sampler);
  vkUpdateDescriptorSets(context_->logical_device, writes_.size(), writes_.data(), 0, nullptr);

  {
    const core::Status status =
        vertex_buffer_staging_.MapData([this](void* data) {
          memcpy(data, skybox_vertices_.data(),
                 sizeof(float) * skybox_vertices_.size());
        });
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status =
        vertex_buffer_staging_.CopyToBuffer(vertex_buffer_local_);
    if (!status.ok()) {
      return status;
    }
  }

  {
    const core::Status status = uniform_buffer_.MapData([](void* data) {
      UniformBufferObject init{glm::mat4(1.0f), glm::mat4(1.0f)};
      memcpy(data, &init, sizeof(UniformBufferObject));
    });
    if (!status.ok()) {
      return status;
    }
  }

  return core::Status();
}

core::Status RenderSkybox::Init(const std::array<std::string, 6>& face_paths) {
  {
    const core::Status status = LoadCubeMap(face_paths);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = CreateBuffers();
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = Init();
    if (!status.ok()) {
      return status;
    }
  }

  return core::Status();
}

void RenderSkybox::Render(VkCommandBuffer command_buffer, VkExtent2D extent) {
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
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                          &descriptor_set_, 0, nullptr);
  vkCmdDraw(command_buffer, 36, 1, 0, 0);
}

core::Status RenderSkybox::UpdateUniformBuffer(const glm::mat4& view,
                                               const glm::mat4& project) {
  {
    const core::Status status = uniform_buffer_.MapData([&](void* data) {
      UniformBufferObject ubo{};
      // Drop the translation component so the skybox follows the camera.
      ubo.view = glm::mat4(glm::mat3(view));
      ubo.project = project;
      ubo.project[1][1] *= -1;  // Vulkan Y-flip
      memcpy(data, &ubo, sizeof(UniformBufferObject));
    });
    if (!status.ok()) {
      return status;
    }
  }

  return core::Status();
}

std::vector<core::vulkan::BindingInfo> RenderSkybox::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
          {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}};
}

const std::vector<uint32_t> RenderSkybox::LoadVertexShader() const {
  static const std::vector<uint32_t> shader_code =
#include "Sky.vert.spv"
      ;
  return shader_code;
}

const std::vector<uint32_t> RenderSkybox::LoadFragmentShader() const {
  static const std::vector<uint32_t> shader_code =
#include "Sky.frag.spv"
      ;
  return shader_code;
}

std::vector<VkVertexInputBindingDescription> RenderSkybox::GetVertexBindingDescriptions() const {
  VkVertexInputBindingDescription b{};
  b.binding = 0;
  b.stride = sizeof(float) * 3;
  b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return {b};
}

std::vector<VkVertexInputAttributeDescription> RenderSkybox::GetVertexAttributeDescriptions()
    const {
  VkVertexInputAttributeDescription a{};
  a.binding = 0;
  a.location = 0;
  a.format = VK_FORMAT_R32G32B32_SFLOAT;
  a.offset = 0;
  return {a};
}

core::Status RenderSkybox::LoadCubeMap(
    const std::array<std::string, 6>& face_paths) {
  // Load all six faces; require identical dimensions so they pack cleanly
  // into one staging buffer.
  int face_w = 0;
  int face_h = 0;
  std::vector<stbi_uc*> face_pixels(6, nullptr);
  for (size_t i = 0; i < 6; ++i) {
    int w = 0;
    int h = 0;
    int c = 0;
    face_pixels[i] = stbi_load(face_paths[i].c_str(), &w, &h, &c, STBI_rgb_alpha);
    if (!face_pixels[i]) {
      return core::Status(core::ErrorCode::kBackend,
                          "failed to load cubemap face: " + face_paths[i]);
    }
    if (i == 0) {
      face_w = w;
      face_h = h;
    } else if (w != face_w || h != face_h) {
      return core::Status(core::ErrorCode::kBackend,
                          "cubemap faces must share the same dimensions");
    }
  }

  const VkDeviceSize face_bytes = static_cast<VkDeviceSize>(face_w) * face_h * 4;
  const VkDeviceSize total_bytes = face_bytes * 6;

  auto staging_buffer_result = core::vulkan::VulkanBuffer::Create(
      context_, total_bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!staging_buffer_result.ok()) {
    return staging_buffer_result.status();
  }
  std::unique_ptr<core::vulkan::VulkanBuffer> staging_buffer_owner =
      std::move(*staging_buffer_result);
  core::vulkan::VulkanBuffer& staging_buffer = *staging_buffer_owner;
  {
    const core::Status status = staging_buffer.MapData([&](void* data) {
      auto* dst = static_cast<uint8_t*>(data);
      for (size_t i = 0; i < 6; ++i) {
        memcpy(dst + i * face_bytes, face_pixels[i],
               static_cast<size_t>(face_bytes));
      }
    });
    if (!status.ok()) {
      return status;
    }
  }
  for (auto* p : face_pixels) stbi_image_free(p);

  {
    const core::Status status = cube_map_image_.Init(
        context_, static_cast<uint32_t>(face_w), static_cast<uint32_t>(face_h),
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_TILING_OPTIMAL, 1, VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, 6);
    if (!status.ok()) {
      return status;
    }
  }

  {
    const core::Status status = cube_map_image_.TransitionImageLayout(
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_FORMAT_R8G8B8A8_SRGB, 1, 6);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = staging_buffer.CopyToImage(
        cube_map_image_, static_cast<uint32_t>(face_w),
        static_cast<uint32_t>(face_h), 6);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = cube_map_image_.TransitionImageLayout(
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_FORMAT_R8G8B8A8_SRGB, 1,
        6);
    if (!status.ok()) {
      return status;
    }
  }

  return core::Status();
}

core::Status RenderSkybox::CreateBuffers() {
  const VkDeviceSize v_size = skybox_vertices_.size() * sizeof(float);

  {
    const core::Status status = vertex_buffer_staging_.Init(
        context_, v_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = vertex_buffer_local_.Init(
        context_, v_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!status.ok()) {
      return status;
    }
  }

  {
    const core::Status status =
        uniform_buffer_.Init(context_, sizeof(UniformBufferObject),
                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!status.ok()) {
      return status;
    }
  }

  return core::Status();
}

}  // namespace core
