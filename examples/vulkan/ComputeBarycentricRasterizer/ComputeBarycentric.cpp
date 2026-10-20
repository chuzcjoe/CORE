#include "ComputeBarycentric.h"

#include "Status.h"

namespace core {
namespace example {

ComputeBarycentric::ComputeBarycentric(core::vulkan::VulkanContext* context,
                                       const int width, const int height)
    : core::vulkan::VulkanCompute(context),
      uniform_data_{.width = width, .height = height} {}

core::Status ComputeBarycentric::Init() {
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
    const core::Status status = CreateBuffers();
    if (!status.ok()) {
      return status;
    }
  }

  {
    const core::Status status = core::vulkan::VulkanCompute::Init();
    if (!status.ok()) {
      return status;
    }
  }

  // Copy uniform data to the uniform buffer
  // Fill uniform with randon A, B, C vertex coordinates
  {
    const core::Status status = uniform_buffer_.MapData([this](void* data) {
      uniform_data_.vertex_a = glm::vec2(300, 800);
      uniform_data_.vertex_b = glm::vec2(500, 100);
      uniform_data_.vertex_c = glm::vec2(700, 800);
      memcpy(data, &uniform_data_, sizeof(UniformData));
    });
    if (!status.ok()) {
      return status;
    }
  }

  // Fill rasterized with all 0s
  {
    const core::Status status = rasterized.MapData([this](void* data) {
      core::Mat<int, 1> mat(uniform_data_.height, uniform_data_.width);
      mat.Fill(0);
      memcpy(data, mat.data(), mat.total() * sizeof(int));
    });
    if (!status.ok()) {
      return status;
    }
  }

  CreateUniformBufferDescriptorSet(0, uniform_buffer_);
  CreateStorageBufferDescriptorSet(1, rasterized);

  vkUpdateDescriptorSets(context_->logical_device, writes_.size(), writes_.data(), 0, nullptr);

  return core::Status();
}

core::Status ComputeBarycentric::Run(const VkCommandBuffer command_buffer) {
  // Record commands to dispatch the compute shader
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1,
                          &descriptor_set_, 0, nullptr);

  const uint32_t group_x = (uniform_data_.width + 15) / 16;
  const uint32_t group_y = (uniform_data_.height + 15) / 16;
  vkCmdDispatch(command_buffer, group_x, group_y, 1);

  return core::Status();
}

std::vector<core::vulkan::BindingInfo> ComputeBarycentric::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
          {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT}};
}

const std::vector<uint32_t>& ComputeBarycentric::LoadShaderCode() const {
  static const std::vector<uint32_t> shader_code =
#include "Barycentric.comp.spv"
      ;
  return shader_code;
}

core::Status ComputeBarycentric::CreateBuffers() {
  const VkDeviceSize size = uniform_data_.width * uniform_data_.height * sizeof(int);
  {
    const core::Status status =
        rasterized.Init(context_, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!status.ok()) {
      return status;
    }
  }

  return core::Status();
}

}  // namespace example
}  // namespace core
