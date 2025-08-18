#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <vector>

#include "VulkanUtils.h"

namespace core {
namespace vulkan {

class VulkanEnv {
 public:
  VulkanEnv(const bool enable_validation_layers = false);
  ~VulkanEnv() = default;

  VkInstance instance = VK_NULL_HANDLE;

 private:
  bool enable_validation_layers_;

  void CreateInstance(const bool enable_validation_layers);
};
}  // namespace vulkan
}  // namespace core
