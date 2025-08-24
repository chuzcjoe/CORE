#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <vector>

#include "VulkanUtils.h"

namespace core {
namespace vulkan {

// Vulkan context managing instance, physical device, logical device, and queues
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
  VkDebugUtilsMessengerEXT debug_messenger_;

  void CreateInstance(const bool enable_validation_layers);
  void PickPhysicalDevice();
  std::optional<uint32_t> FindQueueFamilies(VkPhysicalDevice device);
  void CreateLogicalDevice(const float queuePriority = 1.0f);
  std::vector<const char*> GetRequiredDeviceExtensions() const;
  void SetupDebugMessenger();
};
}  // namespace vulkan
}  // namespace core
