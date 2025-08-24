#include "VulkanContext.h"

#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace core {
namespace vulkan {

VulkanContext::VulkanContext(const bool enable_validation_layers)
    : enable_validation_layers_(enable_validation_layers) {
  CreateInstance(enable_validation_layers_);
  PickPhysicalDevice();
  CreateLogicalDevice();
}

void VulkanContext::CreateInstance(const bool enable_validation_layers) {
  // Get required extensions
  std::vector<const char*> extensions;
  if (enable_validation_layers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

#ifdef __APPLE__
  extensions.emplace_back("VK_KHR_portability_enumeration");
#endif

  // Search for all support extensions
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> support_extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, support_extensions.data());

  // Check if required extensions are supported
  std::unordered_set<std::string> all_extensions;
  for (const auto& extension : support_extensions) {
    all_extensions.emplace(extension.extensionName);
  }

  for (const auto& extension : extensions) {
    if (all_extensions.find(extension) == all_extensions.end()) {
      std::cerr << "Missing Vulkan extension: " << extension << "\n";
      throw std::runtime_error("required Vulkan extension not supported");
    }
  }

  VkApplicationInfo app{};
  app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app.pApplicationName = "CORE";
  app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app.pEngineName = "CORE ENGINE";
  app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ci.pApplicationInfo = &app;
  ci.enabledLayerCount = 1;
  ci.ppEnabledLayerNames = &kValidationLayerName;
#ifdef __APPLE__
  ci.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  ci.enabledExtensionCount = extensions.size();
  ci.ppEnabledExtensionNames = extensions.data();
  ci.pNext = nullptr;

  VK_CHECK(vkCreateInstance(&ci, nullptr, &instance));
}

void VulkanContext::PickPhysicalDevice() {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
  if (device_count == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }
  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

  for (const auto& device : devices) {
    const auto queue_family = FindQueueFamilies(device);
    if (queue_family.has_value()) {
      physical_device_ = device;
      break;
    }
  }

  if (physical_device_ == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

std::optional<uint32_t> VulkanContext::FindQueueFamilies(VkPhysicalDevice device) {
  const VkQueueFlags kTargetFlag = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queueFamilies.data());

  uint32_t i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if ((queueFamily.queueFlags & kTargetFlag) == kTargetFlag) {
      queue_family_ = i;
      return i;
    }
    i++;
  }
  return {};
}

void VulkanContext::CreateLogicalDevice(const float queuePriority) {
  VkDeviceQueueCreateInfo queue_info{};
  queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info.queueFamilyIndex = queue_family_;
  queue_info.queueCount = 1;
  queue_info.pQueuePriorities = &queuePriority;

  // TODO: support more queue family types
  VkDeviceCreateInfo deviceInfo{};
  deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceInfo.pQueueCreateInfos = &queue_info;
  deviceInfo.queueCreateInfoCount = 1;

  const auto extensions = GetRequiredDeviceExtensions();
  deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  deviceInfo.ppEnabledExtensionNames = extensions.data();

  VK_CHECK(vkCreateDevice(physical_device_, &deviceInfo, nullptr, &device_));
}

std::vector<const char*> VulkanContext::GetRequiredDeviceExtensions() const {
  std::vector<const char*> extensions;

  // TODO: add more extensions if needed

#ifdef __APPLE__
  extensions.emplace_back("VK_KHR_portability_subset");
#endif

  // check if required extensions are supported
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &extensionCount,
                                       supportedExtensions.data());

  std::unordered_set<std::string> availableExtensions;
  for (const auto& ext : supportedExtensions) {
    availableExtensions.insert(ext.extensionName);
  }

  for (const auto& ext : extensions) {
    if (availableExtensions.find(ext) == availableExtensions.end()) {
      std::cerr << "Missing device extension: " << ext << std::endl;
      throw std::runtime_error("Required device extension not supported");
    }
  }

  return extensions;
}

}  // namespace vulkan
}  // namespace core