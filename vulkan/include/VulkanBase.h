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

class VulkanBase {
 public:
  explicit VulkanBase(VulkanContext* context);
  virtual ~VulkanBase();

  virtual void Init();

  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;

 protected:
  virtual std::vector<BindingInfo> GetBindingInfo() const = 0;
  virtual void CreatePipeline() = 0;

 protected:
  std::vector<VkDescriptorSetLayoutBinding> CreateDescriptorLayoutBindings() const;
  std::vector<DescriptorPoolInfo> CreateDescriptorPoolSizes() const;
  VkShaderModule CreateShaderModule(const std::vector<uint32_t>& shader_code) const;

  void CreateUniformBufferDescriptorSet(const uint32_t binding, const VulkanBuffer& buffer);
  void CreateStorageBufferDescriptorSet(const uint32_t binding, const VulkanBuffer& buffer);

  VulkanContext* context_;
  VkDescriptorSetLayout descriptor_set_layout_;
  VkDescriptorPool descriptor_pool_;
  VkDescriptorSet descriptor_set_;

  std::vector<VkDescriptorBufferInfo> buffer_infos_;
  std::vector<VkWriteDescriptorSet> writes_;

 private:
  void CheckBufferInfoSize() {
    const auto size = GetBindingInfo().size();
    if (buffer_infos_.size() == 0) {
      buffer_infos_.resize(size);
    }
    if (writes_.size() == 0) {
      writes_.resize(size);
    }
  }
};

}  // namespace vulkan
}  // namespace core