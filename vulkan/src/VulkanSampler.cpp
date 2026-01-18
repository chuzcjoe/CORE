#include "VulkanSampler.h"

namespace core {
namespace vulkan {

VulkanSampler::VulkanSampler(VulkanContext* context) : context_(context) {
  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.anisotropyEnable = VK_FALSE;  // Need to enable device feature first, if set VK_TRUE
  sampler_info.maxAnisotropy = 0;
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias = 0.0f;
  sampler_info.minLod = 0.0f;
  sampler_info.maxLod = VK_LOD_CLAMP_NONE;

  VK_CHECK(vkCreateSampler(context_->logical_device, &sampler_info, nullptr, &sampler));
}

VulkanSampler::~VulkanSampler() {
  if (sampler != VK_NULL_HANDLE) {
    vkDestroySampler(context_->logical_device, sampler, nullptr);
  }
}

VkSampleCountFlagBits VulkanSampler::GetMaxUsableSampleCount() const {
  VkPhysicalDeviceProperties physical_device_properties{};
  vkGetPhysicalDeviceProperties(context_->physical_device, &physical_device_properties);

  VkSampleCountFlags counts = physical_device_properties.limits.framebufferColorSampleCounts &
                              physical_device_properties.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    return VK_SAMPLE_COUNT_2_BIT;
  }

  return VK_SAMPLE_COUNT_1_BIT;
}

}  // namespace vulkan
}  // namespace core