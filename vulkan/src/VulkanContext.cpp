#include "VulkanContext.h"

#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace core {
namespace vulkan {

static const std::vector<const char*> kValidationLayerName = {"VK_LAYER_KHRONOS_validation"};

const char* toStringMessageSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT s) {
  switch (s) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      return "VERBOSE";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      return "ERROR";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      return "WARNING";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      return "INFO";
    default:
      return "UNKNOWN";
  }
}

const char* toStringMessageType(VkDebugUtilsMessageTypeFlagsEXT s) {
  if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
    return "General | Validation | Performance";
  if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
    return "Validation | Performance";
  if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
    return "General | Performance";
  if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)) return "Performance";
  if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT))
    return "General | Validation";
  if (s == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) return "Validation";
  if (s == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) return "General";
  return "Unknown";
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
  auto ms = toStringMessageSeverity(messageSeverity);
  auto mt = toStringMessageType(messageType);
  printf("[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);

  return VK_FALSE;
}

VulkanContext::VulkanContext(const bool enable_validation_layers, QueueFamilyType queue_family_type,
                             const VkSurfaceKHR surface)
    : enable_validation_layers_(enable_validation_layers),
      queue_family_type_(queue_family_type),
      surface_(surface) {
  CreateInstance(enable_validation_layers_);
}

void VulkanContext::Init(VkSurfaceKHR surface) {
  surface_ = surface;
  PickPhysicalDevice(queue_family_type_);
  CreateLogicalDevice();
  SetupDebugMessenger();
}

VulkanContext::~VulkanContext() {
  // if (debug_messenger_ != VK_NULL_HANDLE) {
  //   auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
  //       instance, "vkDestroyDebugUtilsMessengerEXT");
  //   if (func != nullptr) {
  //     func(instance, debug_messenger_, nullptr);
  //   }
  // }

  // if (logical_device != VK_NULL_HANDLE) {
  //   vkDestroyDevice(logical_device, nullptr);
  // }

  // if (instance != VK_NULL_HANDLE) {
  //   vkDestroyInstance(instance, nullptr);
  // }
}

uint32_t VulkanContext::FindMemoryType(const uint32_t type_filter,
                                       const VkMemoryPropertyFlags properties) const {
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
    if ((type_filter & (1 << i)) &&
        (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
  return -1;
}

void VulkanContext::CreateInstance(const bool enable_validation_layers) {
  const bool is_validation_supported = CheckValidationLayerSupport();
  // Get required extensions
  std::vector<const char*> extensions;
  if (enable_validation_layers) {
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  extensions.emplace_back("VK_KHR_surface");

#ifdef __APPLE__
  extensions.emplace_back("VK_KHR_portability_enumeration");
  uint32_t glfw_extension_count = 0;
  const char** glfw_required_extensions;
  glfw_required_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
  printf("glfw extensions count: %d\n", glfw_extension_count);
  std::vector<const char*> glfw_extensions(glfw_required_extensions,
                                           glfw_required_extensions + glfw_extension_count);
  for (uint32_t i = 0; i < glfw_extension_count; ++i) {
    printf("glfw extensions: %s\n", glfw_extensions[i]);
    extensions.emplace_back(glfw_extensions[i]);
  }
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
  app.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ci.pApplicationInfo = &app;
  ci.enabledExtensionCount = extensions.size();
  ci.ppEnabledExtensionNames = extensions.data();
  ci.pNext = nullptr;

#ifdef __APPLE__
  ci.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  if (enable_validation_layers && is_validation_supported) {
    ci.enabledLayerCount = kValidationLayerName.size();
    ci.ppEnabledLayerNames = kValidationLayerName.data();
  } else {
    ci.enabledLayerCount = 0;
    ci.ppEnabledLayerNames = nullptr;
  }

  VK_CHECK(vkCreateInstance(&ci, nullptr, &instance));
}

void VulkanContext::PickPhysicalDevice(const QueueFamilyType queue_family_type) {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
  if (device_count == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }
  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

  for (const auto& device : devices) {
    FindQueueFamilies(device, queue_family_type);
    if (queue_family_indices_.is_complete()) {
      physical_device = device;
      break;
    }
  }

  if (physical_device == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }

  // Get Timestamp period
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(physical_device, &properties);
  const float period = properties.limits.timestampPeriod;
  if (period == 0.0f) {
    std::runtime_error("timestamp is 0");
  }
  timestamp_period = period;
}

void VulkanContext::FindQueueFamilies(VkPhysicalDevice device,
                                      const QueueFamilyType queue_family_type) {
  VkQueueFlags compute_flag = 0;
  VkQueueFlags graphics_flag = 0;

  switch (queue_family_type) {
    case QueueFamilyType::Compute:
      compute_flag = VK_QUEUE_COMPUTE_BIT;
      break;
    case QueueFamilyType::Graphics:
      graphics_flag = VK_QUEUE_GRAPHICS_BIT;
      break;
    default:
      throw std::runtime_error("Currently only support compute and graphics");
  }

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queueFamilies.data());

  uint32_t i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & compute_flag) {
      queue_family_indices_.compute_family = i;
    }
    if (queueFamily.queueFlags & graphics_flag) {
      queue_family_indices_.graphics_family = i;
      VkBool32 present_support = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
      if (present_support) {
        queue_family_indices_.present_family = i;
      }
    }
    if (queue_family_indices_.is_complete()) {
      break;
    }
    ++i;
  }
}

void VulkanContext::CreateLogicalDevice(const float queuePriority) {
  std::vector<VkDeviceQueueCreateInfo> queue_infos;
  // TODO: support present queue
  std::set<std::optional<uint32_t>> unique_queue_families;
  if (queue_family_indices_.compute_family.has_value())
    unique_queue_families.insert(queue_family_indices_.compute_family);
  if (queue_family_indices_.graphics_family.has_value())
    unique_queue_families.insert(queue_family_indices_.graphics_family);
  if (queue_family_indices_.present_family.has_value())
    unique_queue_families.insert(queue_family_indices_.present_family);

  for (auto family : unique_queue_families) {
    if (!family.has_value()) {
      continue;
    }
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = family.value();
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queuePriority;
    queue_infos.emplace_back(queue_info);
  }

  // TODO: support more queue family types
  VkDeviceCreateInfo deviceInfo{};
  deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceInfo.pQueueCreateInfos = queue_infos.data();
  deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());

  const auto extensions = GetRequiredDeviceExtensions();
  deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  deviceInfo.ppEnabledExtensionNames = extensions.data();

  VK_CHECK(vkCreateDevice(physical_device, &deviceInfo, nullptr, &logical_device));

  if (queue_family_indices_.compute_family.has_value()) {
    vkGetDeviceQueue(logical_device, queue_family_indices_.compute_family.value(), 0,
                     &compute_queue_);
  }
  if (queue_family_indices_.graphics_family.has_value()) {
    printf("Found graphics_family\n");
    vkGetDeviceQueue(logical_device, queue_family_indices_.graphics_family.value(), 0,
                     &graphics_queue_);
  }
  if (queue_family_indices_.present_family.has_value()) {
    printf("Found present_family\n");
    vkGetDeviceQueue(logical_device, queue_family_indices_.present_family.value(), 0,
                     &present_queue_);
  }
}

std::vector<const char*> VulkanContext::GetRequiredDeviceExtensions() const {
  std::vector<const char*> extensions;

  // TODO: add more extensions if needed

#ifdef __APPLE__
  extensions.emplace_back("VK_KHR_portability_subset");
#endif

#if defined(__ANDROID__)
  extensions.emplace_back("VK_KHR_external_memory_fd");
#endif

  // more extensions
  extensions.emplace_back("VK_KHR_swapchain");
  extensions.emplace_back("VK_KHR_16bit_storage");

  // check if required extensions are supported
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount,
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

void VulkanContext::SetupDebugMessenger() {
  if (!enable_validation_layers_) return;

  VkDebugUtilsMessengerCreateInfoEXT create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = debugCallback;

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, &create_info, nullptr, &debug_messenger_);
  }
}

bool VulkanContext::CheckValidationLayerSupport() {
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  for (const auto& layer : available_layers) {
    if (strcmp(kValidationLayerName[0], layer.layerName) == 0) {
      return true;
    }
  }
  return false;
}

}  // namespace vulkan
}  // namespace core