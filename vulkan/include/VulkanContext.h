#pragma once

#include <vulkan/vulkan.h>

#include <set>
#include <unordered_map>
#include <vector>

#include "VulkanUtils.h"

namespace core {
namespace vulkan {

struct QueueFamilyIndices {
  std::optional<uint32_t> compute_family;
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;

  bool is_complete = false;
};

// Vulkan context managing instance, physical device, logical device, and queues
class VulkanContext {
 public:
  VulkanContext(const bool enable_validation_layers = false,
                const QueueFamilyType queue_family_type = QueueFamilyType::All);
  ~VulkanContext() = default;

  VkInstance instance = VK_NULL_HANDLE;

 private:
  bool enable_validation_layers_;
  QueueFamilyIndices queue_family_indices_;
  VkQueue compute_queue_;
  VkQueue graphics_queue_;
  VkPhysicalDevice physical_device_;
  VkDevice device_;
  VkDebugUtilsMessengerEXT debug_messenger_;

  void CreateInstance(const bool enable_validation_layers);
  void PickPhysicalDevice(const QueueFamilyType queue_family_type);
  void FindQueueFamilies(VkPhysicalDevice device, const QueueFamilyType queue_family_type);
  void CreateLogicalDevice(const float queuePriority = 1.0f);
  std::vector<const char*> GetRequiredDeviceExtensions() const;
  void SetupDebugMessenger();
};
}  // namespace vulkan
}  // namespace core
