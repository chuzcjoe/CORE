#include "VulkanCompute.h"

namespace core {
namespace vulkan {

VulkanCompute::VulkanCompute(VulkanContext* context) : VulkanBase(context) {}

void VulkanCompute::CreatePipeline() {
  const auto shader_code = LoadShaderCode();
  const VkShaderModule shader_module = CreateShaderModule(shader_code);

  VkPipelineShaderStageCreateInfo shader_stage_info{};
  shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shader_stage_info.module = shader_module;
  shader_stage_info.pName = "main";

  VkComputePipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipeline_info.stage = shader_stage_info;
  pipeline_info.layout = pipeline_layout;

  VK_CHECK(vkCreateComputePipelines(context_->logical_device, VK_NULL_HANDLE, 1, &pipeline_info,
                                    nullptr, &pipeline));

  vkDestroyShaderModule(context_->logical_device, shader_module, nullptr);
}

}  // namespace vulkan
}  // namespace core