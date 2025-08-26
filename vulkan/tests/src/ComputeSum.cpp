#include "ComputeSum.h"

namespace core {
namespace vulkan {

ComputeSum::ComputeSum(VulkanContext* context, VulkanBuffer& src, const int width, const int height)
    : VulkanCompute(context),
      src_buffer(src),
      uniform_buffer_(context, sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
      uniform_data_{.width = width, .height = height} {}

void ComputeSum::Init() {
  VulkanCompute::Init();

  const auto uniform_descriptor = CreateUniformBufferDescriptorSet(0, uniform_buffer_);
  const auto src_descriptor = CreateStorageBufferDescriptorSet(1, src_buffer);

  const std::vector<VkWriteDescriptorSet> descriptor_writes = {uniform_descriptor, src_descriptor};
  vkUpdateDescriptorSets(context_->logical_device, static_cast<uint32_t>(descriptor_writes.size()),
                         descriptor_writes.data(), 0, nullptr);
}

void ComputeSum::Run(const VkCommandBuffer command_buffer) {
  // Copy uniform data to the uniform buffer
  uniform_buffer_.MapData(
      [this](void* data) { memcpy(data, &uniform_data_, sizeof(UniformData)); });

  // Record commands to dispatch the compute shader
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline());
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, PipelineLayout(), 0, 1,
                          &descriptor_set_, 0, nullptr);

  const uint32_t group_x = (uniform_data_.width + 15) / 16;
  const uint32_t group_y = (uniform_data_.height + 15) / 16;
  vkCmdDispatch(command_buffer, group_x, group_y, 1);
}

std::vector<BindingInfo> ComputeSum::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
          {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT}};
}

const std::vector<uint32_t>& ComputeSum::LoadShaderCode() const {
  // Load and return the SPIR-V code for the compute shader
  // This is a placeholder; actual implementation will read from a file or embedded resource
  static const std::vector<uint32_t> shader_code =
#include "ComputeSum.comp.spv"
      ;
  return shader_code;
}

}  // namespace vulkan
}  // namespace core