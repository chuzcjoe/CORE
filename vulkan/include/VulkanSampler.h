#pragma once

#include <vulkan/vulkan.h>

#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanSampler {
 public:
  explicit VulkanSampler(VulkanContext* context);
  ~VulkanSampler();

  VkSampleCountFlagBits GetMaxUsableSampleCount() const;

 private:
  VulkanContext* context_ = nullptr;

 public:
  VkSampler sampler = VK_NULL_HANDLE;
};

}  // namespace vulkan
}  // namespace core