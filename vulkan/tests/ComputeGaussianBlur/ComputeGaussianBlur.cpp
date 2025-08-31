#include "ComputeGaussianBlur.h"

namespace core {
namespace vulkan {

ComputeGaussianBlur::ComputeGaussianBlur(VulkanContext* context, VulkanBuffer& src,
                                         VulkanBuffer& dst, const int width, const int height)
    : VulkanCompute(context),
      src_buffer(src),
      dst_buffer(dst),
      uniform_buffer_(context, sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
      uniform_data_{.width = width, .height = height},
      gaussian_kernel_(context, gaussian_kernel_data_.size() * sizeof(float),
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
}

void ComputeGaussianBlur::Init() {
  VulkanCompute::Init();

  gaussian_kernel_.MapData([this](void* data) {
    memcpy(data, gaussian_kernel_data_.data(), gaussian_kernel_data_.size() * sizeof(float));
  });

  CreateUniformBufferDescriptorSet(0, uniform_buffer_);
  CreateStorageBufferDescriptorSet(1, src_buffer);
  CreateStorageBufferDescriptorSet(2, dst_buffer);
  CreateStorageBufferDescriptorSet(3, gaussian_kernel_);

  vkUpdateDescriptorSets(context_->logical_device, writes_.size(), writes_.data(), 0, nullptr);
}

void ComputeGaussianBlur::Run(const VkCommandBuffer command_buffer) {
  // Copy uniform data to the uniform buffer
  uniform_buffer_.MapData(
      [this](void* data) { memcpy(data, &uniform_data_, sizeof(UniformData)); });

  // Record commands to dispatch the compute shader
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1,
                          &descriptor_set_, 0, nullptr);

  const uint32_t group_x = (uniform_data_.width + 15) / 16;
  const uint32_t group_y = (uniform_data_.height + 15) / 16;
  vkCmdDispatch(command_buffer, group_x, group_y, 1);
}

std::vector<BindingInfo> ComputeGaussianBlur::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
          {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
          {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
          {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT}};
}

const std::vector<uint32_t>& ComputeGaussianBlur::LoadShaderCode() const {
  // Load and return the SPIR-V code for the compute shader
  // This is a placeholder; actual implementation will read from a file or embedded resource
  static const std::vector<uint32_t> shader_code =
#include "ComputeGaussianBlur.comp.spv"
      ;
  return shader_code;
}

}  // namespace vulkan
}  // namespace core