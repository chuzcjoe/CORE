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

/*
Vulkan context manages:
1. instance
2. physical device
3. logical device
4. queue families
5. queues
6. debug messenger
*/

class VulkanContext {
 public:
  VulkanContext(const bool enable_validation_layers = false,
                const QueueFamilyType queue_family_type = QueueFamilyType::All);
  ~VulkanContext();

  uint32_t FindMemoryType(const uint32_t type_filter, const VkMemoryPropertyFlags properties) const;

  QueueFamilyIndices GetQueueFamilyIndices() const { return queue_family_indices_; }

  VkQueue compute_queue() const { return compute_queue_; }
  VkQueue graphics_queue() const { return graphics_queue_; }

  VkInstance instance = VK_NULL_HANDLE;
  VkDevice logical_device;
  float timestamp_period;

 private:
  bool enable_validation_layers_;
  QueueFamilyIndices queue_family_indices_;
  VkQueue compute_queue_;
  VkQueue graphics_queue_;
  VkPhysicalDevice physical_device_;
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
