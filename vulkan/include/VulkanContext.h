#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <vector>

#include "VulkanUtils.h"

namespace core {
namespace vulkan {

static constexpr const char* kValidationLayerName = "VK_LAYER_KHRONOS_validation";

class VulkanContext {
 public:
  VulkanContext(const bool enable_validation_layers = false);
  ~VulkanContext() = default;

  VkInstance instance = VK_NULL_HANDLE;

 private:
  bool enable_validation_layers_;
  uint32_t queue_family_;
  VkPhysicalDevice physical_device_;
  VkDevice device_;

  void CreateInstance(const bool enable_validation_layers);
  void PickPhysicalDevice();
  std::optional<uint32_t> FindQueueFamilies(VkPhysicalDevice device);
  void CreateLogicalDevice(const float queuePriority = 1.0f);
  std::vector<const char*> GetRequiredDeviceExtensions() const;
};
}  // namespace vulkan
}  // namespace core
