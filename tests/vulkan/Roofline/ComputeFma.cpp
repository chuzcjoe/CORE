#include "ComputeFma.h"

#include <cstring>

#include "Status.h"

namespace core {
namespace vulkan {

ComputeFma::ComputeFma(VulkanContext* context, VulkanBuffer& dst, const int iterations,
                       const uint32_t group_count)
    : VulkanCompute(context),
      dst_buffer_(dst),
      uniform_data_{.iterations = iterations},
      group_count_(group_count) {}

core::Status ComputeFma::Init() {
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
  CreateStorageBufferDescriptorSet(1, dst_buffer_);

  vkUpdateDescriptorSets(context_->logical_device, writes_.size(), writes_.data(), 0, nullptr);

  return core::Status();
}

core::Status ComputeFma::Run(const VkCommandBuffer command_buffer) {
  {
    const core::Status status = uniform_buffer_.MapData([this](void* data) {
      memcpy(data, &uniform_data_, sizeof(UniformData));
    });
    if (!status.ok()) {
      return status;
    }
  }

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1,
                          &descriptor_set_, 0, nullptr);
  vkCmdDispatch(command_buffer, group_count_, 1, 1);

  return core::Status();
}

std::vector<BindingInfo> ComputeFma::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
          {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT}};
}

const std::vector<uint32_t>& ComputeFma::LoadShaderCode() const {
  static const std::vector<uint32_t> shader_code =
#include "ComputeFma.comp.spv"
      ;
  return shader_code;
}

}  // namespace vulkan
}  // namespace core
