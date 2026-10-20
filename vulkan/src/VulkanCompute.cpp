#include "VulkanCompute.h"

#include <fstream>
#include <memory>
#include <type_traits>

#include "Status.h"

namespace core {
namespace vulkan {

VulkanCompute::VulkanCompute(VulkanContext* context) : VulkanBase(context) {}

VulkanCompute::~VulkanCompute() {
  if (context_ == nullptr || context_->logical_device == VK_NULL_HANDLE) {
    return;
  }

  vkDestroyPipelineCache(context_->logical_device, pipeline_cache_, nullptr);
}

Status VulkanCompute::CreatePipeline() {
  const auto shader_code = LoadShaderCode();

  const auto shader_module_result = CreateShaderModule(shader_code);
  if (!shader_module_result.ok()) {
    return shader_module_result.status();
  }
  const VkShaderModule shader_module = *shader_module_result;
  const auto release_shader_module = [this](VkShaderModule handle) {
    if (handle != VK_NULL_HANDLE) {
      vkDestroyShaderModule(context_->logical_device, handle, nullptr);
    }
  };
  const std::unique_ptr<std::remove_pointer_t<VkShaderModule>,
                        decltype(release_shader_module)>
      shader_module_owner(shader_module, release_shader_module);

  {
    const core::Status status = CreatePipelineCache(GetPipelineCache());
    if (!status.ok()) {
      return status;
    }
  }

  VkPipelineShaderStageCreateInfo shader_stage_info{};
  shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shader_stage_info.module = shader_module;
  shader_stage_info.pName = "main";

  VkComputePipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipeline_info.stage = shader_stage_info;
  pipeline_info.layout = pipeline_layout;

  {
    const VkResult result =
        vkCreateComputePipelines(context_->logical_device, pipeline_cache_, 1,
                                 &pipeline_info, nullptr, &pipeline);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

const std::string VulkanCompute::GetPipelineCache() const { return ""; }

std::vector<char> VulkanCompute::LoadPipelineCache(const std::string& cache) {
  // Load the pipeline cache from the specified file
  std::ifstream file(cache, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    return {};
  }

  size_t file_size = static_cast<size_t>(file.tellg());
  file.seekg(0);

  std::vector<char> buffer(file_size);
  file.read(buffer.data(), file_size);
  return buffer;
}

Status VulkanCompute::CreatePipelineCache(const std::string& cache) {
  std::vector<char> cache_data = LoadPipelineCache(cache);
  VkPipelineCacheCreateInfo cache_info{};
  cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

  if (cache_data.empty()) {
    cache_info.initialDataSize = 0;
    cache_info.pInitialData = nullptr;
  } else {
    cache_info.initialDataSize = cache_data.size();
    cache_info.pInitialData = cache_data.data();
  }

  {
    const VkResult result = vkCreatePipelineCache(
        context_->logical_device, &cache_info, nullptr, &pipeline_cache_);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

Status VulkanCompute::SavePipelineCache(const std::string& cache) const {
  if (pipeline_cache_ == VK_NULL_HANDLE) {
    return Status();
  }

  size_t cache_size = 0;
  {
    const VkResult result = vkGetPipelineCacheData(
        context_->logical_device, pipeline_cache_, &cache_size, nullptr);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }
  if (cache_size == 0) {
    return Status();
  }

  std::vector<char> cache_data(cache_size);
  {
    const VkResult result =
        vkGetPipelineCacheData(context_->logical_device, pipeline_cache_,
                               &cache_size, cache_data.data());
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  std::ofstream file(cache, std::ios::binary);
  if (file.is_open()) {
    file.write(cache_data.data(), cache_data.size());
  }

  return Status();
}

}  // namespace vulkan
}  // namespace core
