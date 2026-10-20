#pragma once

#ifdef __APPLE__
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vulkan/vulkan.h>

#include <memory>
#include <optional>
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

  bool is_complete() {
    return compute_family.has_value() ||
           (graphics_family.has_value() && present_family.has_value());
  }
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
  static Result<std::unique_ptr<VulkanContext>> Create(
      const bool enable_validation_layers = false,
      const QueueFamilyType queue_family_type = QueueFamilyType::Compute,
      const VkSurfaceKHR surface = VK_NULL_HANDLE);
  ~VulkanContext();
  VulkanContext(const VulkanContext&) = delete;
  VulkanContext& operator=(const VulkanContext&) = delete;

  Status Init(VkSurfaceKHR surface = VK_NULL_HANDLE);

  Result<uint32_t> FindMemoryType(const uint32_t type_filter,
                                  const VkMemoryPropertyFlags properties) const;

  QueueFamilyIndices GetQueueFamilyIndices() const { return queue_family_indices_; }

  bool is_initialized() const { return device_ready_; }

  VkQueue compute_queue() const { return compute_queue_; }
  VkQueue graphics_queue() const { return graphics_queue_; }
  VkQueue present_queue() const { return present_queue_; }
  QueueFamilyType queue_family_type() const { return queue_family_type_; }

  VkInstance instance = VK_NULL_HANDLE;
  VkDevice logical_device = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  float timestamp_period = 0.0f;

 private:
  VulkanContext() = default;
  bool device_init_attempted_ = false;
  bool device_ready_ = false;
  bool enable_validation_layers_;
  QueueFamilyType queue_family_type_;
  QueueFamilyIndices queue_family_indices_;
  VkQueue compute_queue_ = VK_NULL_HANDLE;
  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue present_queue_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;

  VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;

  bool CheckValidationLayerSupport();
  Status CreateInstance(const bool enable_validation_layers);
  Status PickPhysicalDevice(const QueueFamilyType queue_family_type);
  Status FindQueueFamilies(VkPhysicalDevice device,
                           const QueueFamilyType queue_family_type);
  Status CreateLogicalDevice(const float queuePriority = 1.0f);
  Result<std::vector<const char*>> GetRequiredDeviceExtensions() const;
  Status SetupDebugMessenger();
};
}  // namespace vulkan
}  // namespace core
