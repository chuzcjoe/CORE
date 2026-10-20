#include "ComputeGaussianBlur.h"

#include <filesystem>

#include "ComputeGaussianBlur.slang.spv.h"
#include "Status.h"
#include "VulkanUtils.h"

namespace core {
namespace vulkan {

ComputeGaussianBlur::ComputeGaussianBlur(VulkanContext* context,
                                         VulkanBuffer& src, VulkanBuffer& dst,
                                         const int width, const int height)
    : VulkanCompute(context),
      src_buffer(src),
      dst_buffer(dst),
      uniform_data_{.width = width, .height = height} {}

core::Status ComputeGaussianBlur::Init() {
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
    const core::Status status = gaussian_kernel_.Init(
        context_, gaussian_kernel_data_.size() * sizeof(float),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
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

  {
    const core::Status status = gaussian_kernel_.MapData([this](void* data) {
      memcpy(data, gaussian_kernel_data_.data(),
             gaussian_kernel_data_.size() * sizeof(float));
    });
    if (!status.ok()) {
      return status;
    }
  }

  CreateUniformBufferDescriptorSet(0, uniform_buffer_);
  CreateStorageBufferDescriptorSet(1, src_buffer);
  CreateStorageBufferDescriptorSet(2, dst_buffer);
  CreateStorageBufferDescriptorSet(3, gaussian_kernel_);

  vkUpdateDescriptorSets(context_->logical_device, writes_.size(), writes_.data(), 0, nullptr);

  // Save cache if file doesn't exist
  if (!std::filesystem::exists(GetPipelineCache())) {
    {
      const core::Status status = SavePipelineCache(GetPipelineCache());
      if (!status.ok()) {
        return status;
      }
    }
  }

  return core::Status();
}

core::Status ComputeGaussianBlur::Run(const VkCommandBuffer command_buffer) {
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

std::vector<BindingInfo> ComputeGaussianBlur::GetBindingInfo() const {
  return {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
          {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
          {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
          {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT}};
}

const std::vector<uint32_t>& ComputeGaussianBlur::LoadShaderCode() const {
  static const std::vector<uint32_t> shader_code = [] {
    std::vector<uint32_t> code(ComputeGaussianBlur_slang_spv_len / sizeof(uint32_t));
    memcpy(code.data(), ComputeGaussianBlur_slang_spv, ComputeGaussianBlur_slang_spv_len);
    return code;
  }();
  return shader_code;
}

const std::string ComputeGaussianBlur::GetPipelineCache() const {
  const std::string cache_dir = PIPELINE_CACHE_DIR;
  if (cache_dir.empty()) {
    return "";
  }
  std::string pipeline_cache = cache_dir + "/gaussian_blur.cache";
  printf("Using pipeline cache file: %s\n", pipeline_cache.c_str());
  return pipeline_cache;
}

}  // namespace vulkan
}  // namespace core
