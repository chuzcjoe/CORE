#include "ComputeSum.h"

#include "Status.h"

namespace core {
namespace vulkan {

ComputeSum::ComputeSum(VulkanContext* context, VulkanBuffer& src, VulkanBuffer& dst,
                       const int width, const int height)
    : VulkanCompute(context),
      src_buffer(src),
      dst_buffer(dst),
      uniform_data_{.width = width, .height = height} {}

core::Status ComputeSum::Init() {
  {
    const core::Status status = uniform_buffer_.Init(
        context_, sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!status.ok()) {
      return status;
    }
  }

  {
    const core::Status status = VulkanCompute::Init();
    if (!status.ok()) {
      return status;
    }
  }

  CreateUniformBufferDescriptorSet(0, uniform_buffer_);
  CreateStorageBufferDescriptorSet(1, src_buffer);
  CreateStorageBufferDescriptorSet(2, dst_buffer);

  vkUpdateDescriptorSets(context_->logical_device, writes_.size(), writes_.data(), 0, nullptr);

  return core::Status();
}

core::Status ComputeSum::Run(const VkCommandBuffer command_buffer) {
  // Copy uniform data to the uniform buffer
  {
    const core::Status status = uniform_buffer_.MapData([this](void* data) {
      memcpy(data, &uniform_data_, sizeof(UniformData));
    });
    if (!status.ok()) {
      return status;
    }
  }

  // Record commands to dispatch the compute shader
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1,
                          &descriptor_set_, 0, nullptr);

  const uint32_t group_x = (uniform_data_.width + 15) / 16;
  const uint32_t group_y = (uniform_data_.height + 15) / 16;
  vkCmdDispatch(command_buffer, group_x, group_y, 1);

  return core::Status();
}

std::vector<BindingInfo> ComputeSum::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
          {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
          {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT}};
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
