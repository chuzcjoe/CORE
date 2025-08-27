#include "VulkanPipeline.h"

namespace core {
namespace vulkan {

// Descriptor visualization: https://chuzcjoe.github.io/CGV/cgv-vulkan-visualization/

VulkanPipeline::VulkanPipeline(VulkanContext* context) : context_(context) {}

VulkanPipeline::~VulkanPipeline() {
  vkDestroyPipeline(context_->logical_device, pipeline, nullptr);
  vkDestroyPipelineLayout(context_->logical_device, pipeline_layout, nullptr);
  vkDestroyDescriptorPool(context_->logical_device, descriptor_pool_, nullptr);
  vkDestroyDescriptorSetLayout(context_->logical_device, descriptor_set_layout_, nullptr);
}

void VulkanPipeline::Init() {
  // 1. Create descriptor set layout
  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  const auto binding_info = CreateDescriptorLayoutBindings();
  layout_info.bindingCount = static_cast<uint32_t>(binding_info.size());
  layout_info.pBindings = binding_info.data();
  VK_CHECK(vkCreateDescriptorSetLayout(context_->logical_device, &layout_info, nullptr,
                                       &descriptor_set_layout_));

  // 2. Create descriptor sets
  const auto pool_info = CreateDescriptorPoolSizes();
  std::vector<VkDescriptorPoolSize> pool_sizes;
  for (const auto& info : pool_info) {
    VkDescriptorPoolSize pool_size{};
    pool_size.type = info.type;
    pool_size.descriptorCount = info.count;
    pool_sizes.emplace_back(pool_size);
  }
  VkDescriptorPoolCreateInfo pool_create_info{};
  pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
  pool_create_info.pPoolSizes = pool_sizes.data();
  pool_create_info.maxSets = 1;
  VK_CHECK(vkCreateDescriptorPool(context_->logical_device, &pool_create_info, nullptr,
                                  &descriptor_pool_));

  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = descriptor_pool_;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &descriptor_set_layout_;
  VK_CHECK(vkAllocateDescriptorSets(context_->logical_device, &alloc_info, &descriptor_set_));

  // 3. Create pipeline layout
  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;
  pipeline_layout_info.pushConstantRangeCount = 0;
  pipeline_layout_info.pPushConstantRanges = nullptr;
  VK_CHECK(vkCreatePipelineLayout(context_->logical_device, &pipeline_layout_info, nullptr,
                                  &pipeline_layout));

  // 4. Create pipeline
  CreatePipeline();
}

std::vector<VkDescriptorSetLayoutBinding> VulkanPipeline::CreateDescriptorLayoutBindings() const {
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  const auto binding_info = GetBindingInfo();
  for (const auto& info : binding_info) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = info.binding;
    binding.descriptorType = info.descriptor_type;
    binding.descriptorCount = 1;  // Assuming one descriptor per binding
    binding.stageFlags = info.shader_stage;
    binding.pImmutableSamplers = nullptr;  // Optional
    bindings.emplace_back(binding);
  }
  return bindings;
}

std::vector<DescriptorPoolInfo> VulkanPipeline::CreateDescriptorPoolSizes() const {
  std::vector<DescriptorPoolInfo> pool_info;
  const auto binding_info = GetBindingInfo();
  std::unordered_map<VkDescriptorType, uint32_t> type_count;
  for (const auto& info : binding_info) {
    type_count[info.descriptor_type]++;
  }
  for (const auto& [type, count] : type_count) {
    DescriptorPoolInfo info{type, count};
    pool_info.emplace_back(info);
  }
  return pool_info;
}

VkShaderModule VulkanPipeline::CreateShaderModule(const std::vector<uint32_t>& shader_code) const {
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = shader_code.size() * sizeof(uint32_t);
  create_info.pCode = shader_code.data();

  VkShaderModule shader_module;
  VK_CHECK(vkCreateShaderModule(context_->logical_device, &create_info, nullptr, &shader_module));
  return shader_module;
}

void VulkanPipeline::CreateUniformBufferDescriptorSet(const uint32_t binding,
                                                      const VulkanBuffer& buffer) {
  CheckBufferInfoSize();
  auto& bi = buffer_infos_[static_cast<int>(binding)];
  bi.buffer = buffer.buffer;
  bi.offset = 0;
  bi.range = buffer.Size();

  VkWriteDescriptorSet w{};
  w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  w.dstSet = descriptor_set_;
  w.dstBinding = binding;
  w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  w.descriptorCount = 1;
  w.pBufferInfo = &bi;
  writes_[static_cast<int>(binding)] = w;
}

void VulkanPipeline::CreateStorageBufferDescriptorSet(const uint32_t binding,
                                                      const VulkanBuffer& buffer) {
  CheckBufferInfoSize();
  auto& bi = buffer_infos_[static_cast<int>(binding)];
  bi.buffer = buffer.buffer;
  bi.offset = 0;
  bi.range = buffer.Size();

  VkWriteDescriptorSet w{};
  w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  w.dstSet = descriptor_set_;
  w.dstBinding = binding;
  w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  w.descriptorCount = 1;
  w.pBufferInfo = &bi;
  writes_[static_cast<int>(binding)] = w;
}

}  // namespace vulkan
}  // namespace core
