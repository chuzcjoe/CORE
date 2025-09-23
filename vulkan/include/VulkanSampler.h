#pragma once

#include <vulkan/vulkan.h>

#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanSampler {
 public:
  explicit VulkanSampler(VulkanContext* context);
  ~VulkanSampler();

 private:
  VulkanContext* context_;

 public:
  VkSampler sampler;
};

}  // namespace vulkan
}  // namespace core