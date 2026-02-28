#include "VulkanCompute.h"

#include <fstream>

namespace core {
namespace vulkan {

VulkanCompute::VulkanCompute(VulkanContext* context) : VulkanBase(context) {}

VulkanCompute::~VulkanCompute() {
  vkDestroyPipelineCache(context_->logical_device, pipeline_cache_, nullptr);
}

void VulkanCompute::CreatePipeline() {
  const auto shader_code = LoadShaderCode();
  const VkShaderModule shader_module = CreateShaderModule(shader_code);

  CreatePipelineCache(GetPipelineCache());

  VkPipelineShaderStageCreateInfo shader_stage_info{};
  shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shader_stage_info.module = shader_module;
  shader_stage_info.pName = "main";

  VkComputePipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipeline_info.stage = shader_stage_info;
  pipeline_info.layout = pipeline_layout;

  VK_CHECK(vkCreateComputePipelines(context_->logical_device, pipeline_cache_, 1, &pipeline_info,
                                    nullptr, &pipeline));

  vkDestroyShaderModule(context_->logical_device, shader_module, nullptr);
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

void VulkanCompute::CreatePipelineCache(const std::string& cache) {
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

  VK_CHECK(vkCreatePipelineCache(context_->logical_device, &cache_info, nullptr, &pipeline_cache_));
}

void VulkanCompute::SavePipelineCache(const std::string& cache) const {
  if (pipeline_cache_ == VK_NULL_HANDLE) {
    return;
  }

  size_t cache_size = 0;
  VK_CHECK(vkGetPipelineCacheData(context_->logical_device, pipeline_cache_, &cache_size, nullptr));
  if (cache_size == 0) {
    return;
  }

  std::vector<char> cache_data(cache_size);
  VK_CHECK(vkGetPipelineCacheData(context_->logical_device, pipeline_cache_, &cache_size,
                                  cache_data.data()));

  std::ofstream file(cache, std::ios::binary);
  if (file.is_open()) {
    file.write(cache_data.data(), cache_data.size());
  }
}

}  // namespace vulkan
}  // namespace core