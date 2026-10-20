#include "VulkanSampler.h"

namespace core {
namespace vulkan {

Result<std::unique_ptr<VulkanSampler>> VulkanSampler::Create(
    VulkanContext* context) {
  std::unique_ptr<VulkanSampler> object(new VulkanSampler());
  const Status status = object->Init(context);
  if (!status.ok()) {
    return Result<std::unique_ptr<VulkanSampler>>(status);
  }
  return Result<std::unique_ptr<VulkanSampler>>(std::move(object));
}

Status VulkanSampler::Init(VulkanContext* context) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanSampler initialization is single-use");
  }
  initialized_ = true;
  context_ = context;
  if (context_ == nullptr || !context_->is_initialized()) {
    return Status(ErrorCode::kInvalidState, "A live Vulkan device is required");
  }

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

  {
    const VkResult result = vkCreateSampler(context_->logical_device,
                                            &sampler_info, nullptr, &sampler);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

VulkanSampler::~VulkanSampler() {
  if (context_ == nullptr || context_->logical_device == VK_NULL_HANDLE) {
    return;
  }

  if (sampler != VK_NULL_HANDLE) {
    vkDestroySampler(context_->logical_device, sampler, nullptr);
  }
}

}  // namespace vulkan
}  // namespace core
