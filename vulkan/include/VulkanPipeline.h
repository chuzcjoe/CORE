#pragma once

#include <vector>

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"

namespace core {
namespace vulkan {

struct BindingInfo {
  uint32_t binding;
  VkDescriptorType descriptor_type;
  VkShaderStageFlagBits shader_stage;
};

struct DescriptorPoolInfo {
  VkDescriptorType type;
  uint32_t count;
};

class VulkanPipeline {
 public:
  explicit VulkanPipeline(VulkanContext* context);
  virtual ~VulkanPipeline();

  virtual void Init();

  VkPipeline Pipeline() const { return pipeline_; }
  VkPipelineLayout PipelineLayout() const { return pipeline_layout_; }

 protected:
  virtual std::vector<BindingInfo> GetBindingInfo() const = 0;
  virtual void CreatePipeline() = 0;

 protected:
  std::vector<VkDescriptorSetLayoutBinding> CreateDescriptorLayoutBindings() const;
  std::vector<DescriptorPoolInfo> CreateDescriptorPoolSizes() const;
  VkShaderModule CreateShaderModule(const std::vector<uint32_t>& shader_code) const;

  VkWriteDescriptorSet CreateUniformBufferDescriptorSet(const uint32_t binding,
                                                        const VulkanBuffer& buffer) const;
  VkWriteDescriptorSet CreateStorageBufferDescriptorSet(const uint32_t binding,
                                                        const VulkanBuffer& buffer) const;

  VulkanContext* context_;
  VkDescriptorSetLayout descriptor_set_layout_;
  VkDescriptorPool descriptor_pool_;
  VkDescriptorSet descriptor_set_;
  VkPipelineLayout pipeline_layout_;
  VkPipeline pipeline_;
};

}  // namespace vulkan
}  // namespace core