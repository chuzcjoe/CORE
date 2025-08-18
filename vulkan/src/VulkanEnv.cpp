#include "VulkanEnv.h"

#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace core {
namespace vulkan {

VulkanEnv::VulkanEnv(const bool enable_validation_layers)
    : enable_validation_layers_(enable_validation_layers) {
  CreateInstance(enable_validation_layers_);
}

void VulkanEnv::CreateInstance(const bool enable_validation_layers) {
  // Get required extensions
  std::vector<const char*> extensions;
  if (enable_validation_layers) {
    extensions.push_back("VK_EXT_debug_utils");
  }

  // Search for all support extensions
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> supportExtensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportExtensions.data());

  // Check if required extensions are supported
  std::unordered_set<std::string> all_extensions;
  ;
  for (const auto& extension : supportExtensions) {
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
  app.pApplicationName = "Minimal";
  app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app.pEngineName = "none";
  app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app.apiVersion = VK_API_VERSION_1_2;  // or 1_1, 1_0

  // Start with no layers/extensions.
  VkInstanceCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ci.pApplicationInfo = &app;
  ci.enabledLayerCount = 0;
  ci.ppEnabledLayerNames = nullptr;
  ci.enabledExtensionCount = extensions.size();
  ci.ppEnabledExtensionNames = extensions.data();
  ci.pNext = nullptr;

  VK_CHECK(vkCreateInstance(&ci, nullptr, &instance));
}

}  // namespace vulkan
}  // namespace core