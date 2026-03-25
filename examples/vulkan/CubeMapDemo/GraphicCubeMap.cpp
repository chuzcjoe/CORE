#include "GraphicCubeMap.h"

namespace core {

GraphicCubeMap::GraphicCubeMap(core::vulkan::VulkanContext* context,
                               const core::vulkan::DynamicRenderingInfo& dynamic_rendering_info)
    : core::vulkan::VulkanGraphic(context, dynamic_rendering_info), sampler_(context) {}

void GraphicCubeMap::Init() {
  core::vulkan::VulkanGraphic::Init();
  // TODO: create descriptor set for cube map
}

void GraphicCubeMap::Render(VkCommandBuffer command_buffer, VkExtent2D extent) {
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
}

const std::vector<core::vulkan::BindingInfo> GraphicCubeMap::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
          {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}};
}

const std::vector<uint32_t> GraphicCubeMap::LoadVertexShader() const {
  static const std::vector<uint32_t> shader_code =
#include "CubeMap.vert.spv"
      ;
  return shader_code;
}

const std::vector<uint32_t> GraphicCubeMap::LoadFragmentShader() const {
  static const std::vector<uint32_t> shader_code =
#include "CubeMap.frag.spv"
      ;
  return shader_code;
}

std::vector<VkVertexInputBindingDescription> GraphicCubeMap::GetVertexBindingDescriptions() const {
  VkVertexInputBindingDescription binding_description{};
  binding_description.binding = 0;
  binding_description.stride = sizeof(Vertex);
  binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return {binding_description};
}

std::vector<VkVertexInputAttributeDescription> GraphicCubeMap::GetVertexAttributeDescriptions()
    const {
  VkVertexInputAttributeDescription attribute_description{};
  attribute_description.binding = 0;
  attribute_description.location = 0;
  attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
  attribute_description.offset = offsetof(Vertex, pos);

  return {attribute_description};
}

void GraphicCubeMap::CreateTextureImage(const std::string& image_path) {
  int texture_width, texture_height, texture_channels;
  const float* pixels =
      stbi_loadf(image_path.c_str(), &texture_width, &texture_height, nullptr, &texture_channels);
  if (!pixels) {
    throw std::runtime_error("failed to load cube map image!");
  }

  // Load and process cubemap
  core::io::Bitmap in(texture_width, texture_height, texture_channels,
                      BitmapFormat::BitmapFormat_Float, pixels);
  core::io::Bitmap out = core::io::ConvertVerticalCrossToCubeMapFaces(in);
  printf("Processed cube map image: width=%d, height=%d, channels=%d\n", out.width, out.height,
         out.depth);

  VkDeviceSize image_size = out.width * out.height * out.depth * sizeof(float);

  core::vulkan::VulkanBuffer staging_buffer(
      context_, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  staging_buffer.MapData([&out, image_size](void* data) {
    memcpy(data, out.pixel.data(), static_cast<size_t>(image_size));
  });

  stbi_image_free(pixels);

  cube_map_image_ =
      core::vulkan::VulkanImage(context_, out.width, out.height, VK_FORMAT_R32G32B32A32_SFLOAT,
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                    VK_IMAGE_USAGE_SAMPLED_BIT,
                                VK_IMAGE_ASPECT_COLOR_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                VK_IMAGE_TILING_OPTIMAL, 1, VK_SAMPLE_COUNT_1_BIT,
                                VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,  // cube map flag
                                6                                     // layers
      );

  cube_map_image_.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        VK_FORMAT_R32G32B32A32_SFLOAT, 1, 6);
  staging_buffer.CopyToImage(cube_map_image_, static_cast<uint32_t>(out.width),
                             static_cast<uint32_t>(out.height), static_cast<uint32_t>(out.depth),
                             6);
  cube_map_image_.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                        VK_FORMAT_R32G32B32A32_SFLOAT, 1, 6);
}

}  // namespace core