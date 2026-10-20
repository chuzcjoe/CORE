#include "VulkanContext.h"

#include <cstring>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "Status.h"

namespace core {
namespace vulkan {

static const std::vector<const char*> kValidationLayerName = {"VK_LAYER_KHRONOS_validation"};

static bool SupportsDynamicRendering(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(device, &props);

  const bool api_13 =
      VK_VERSION_MAJOR(props.apiVersion) > 1 ||
      (VK_VERSION_MAJOR(props.apiVersion) == 1 && VK_VERSION_MINOR(props.apiVersion) >= 3);

  bool has_khr_dynamic_rendering = false;
  if (!api_13) {
    uint32_t ext_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, nullptr);
    std::vector<VkExtensionProperties> exts(ext_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, exts.data());
    for (const auto& ext : exts) {
      if (std::string(ext.extensionName) == VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) {
        has_khr_dynamic_rendering = true;
        break;
      }
    }
  }

  if (!api_13 && !has_khr_dynamic_rendering) {
    return false;
  }

  VkPhysicalDeviceDynamicRenderingFeaturesKHR dyn{};
  dyn.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
  VkPhysicalDeviceFeatures2 features2{};
  features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features2.pNext = &dyn;
  vkGetPhysicalDeviceFeatures2(device, &features2);
  return dyn.dynamicRendering == VK_TRUE;
}

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

Result<std::unique_ptr<VulkanContext>> VulkanContext::Create(
    const bool enable_validation_layers, QueueFamilyType queue_family_type,
    const VkSurfaceKHR surface) {
  std::unique_ptr<VulkanContext> context(new VulkanContext());
  context->enable_validation_layers_ = enable_validation_layers;
  context->queue_family_type_ = queue_family_type;
  context->surface_ = surface;
  const Status status = context->CreateInstance(enable_validation_layers);
  if (!status.ok()) {
    return Result<std::unique_ptr<VulkanContext>>(status);
  }
  return Result<std::unique_ptr<VulkanContext>>(std::move(context));
}

Status VulkanContext::Init(VkSurfaceKHR surface) {
  if (device_init_attempted_) {
    return Status(ErrorCode::kInvalidState,
                  "Vulkan device initialization is single-use");
  }
  device_init_attempted_ = true;
  surface_ = surface;
  {
    const core::Status status = PickPhysicalDevice(queue_family_type_);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = CreateLogicalDevice();
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = SetupDebugMessenger();
    if (!status.ok()) {
      return status;
    }
  }
  device_ready_ = true;
  return Status();
}

VulkanContext::~VulkanContext() {
  if (debug_messenger_ != VK_NULL_HANDLE) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(instance, debug_messenger_, nullptr);
    }
  }

  if (logical_device != VK_NULL_HANDLE) {
    vkDestroyDevice(logical_device, nullptr);
  }

  if (instance != VK_NULL_HANDLE) {
    vkDestroyInstance(instance, nullptr);
  }
}

Result<uint32_t> VulkanContext::FindMemoryType(
    const uint32_t type_filter, const VkMemoryPropertyFlags properties) const {
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
    if ((type_filter & (1 << i)) &&
        (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
      return Result<uint32_t>(i);
    }
  }

  return Result<uint32_t>(
      Status(ErrorCode::kUnsupported, "failed to find suitable memory type!"));
}

Status VulkanContext::CreateInstance(const bool enable_validation_layers) {
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
  std::vector<const char*> glfw_extensions(glfw_required_extensions,
                                           glfw_required_extensions + glfw_extension_count);
  for (uint32_t i = 0; i < glfw_extension_count; ++i) {
    extensions.emplace_back(glfw_extensions[i]);
  }
#endif

  // Search for all support extensions
  uint32_t extensionCount = 0;

  {
    const VkResult error = vkEnumerateInstanceExtensionProperties(
        nullptr, &extensionCount, nullptr);
    if (error != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(error), "Vulkan",
                    error);
    }
  }
  std::vector<VkExtensionProperties> support_extensions(extensionCount);

  {
    const VkResult error = vkEnumerateInstanceExtensionProperties(
        nullptr, &extensionCount, support_extensions.data());
    if (error != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(error), "Vulkan",
                    error);
    }
  }

  // Check if required extensions are supported
  std::unordered_set<std::string> all_extensions;
  for (const auto& extension : support_extensions) {
    all_extensions.emplace(extension.extensionName);
  }

  for (const auto& extension : extensions) {
    if (all_extensions.find(extension) == all_extensions.end()) {
      return Status(
          ErrorCode::kUnsupported,
          std::string("Required Vulkan extension not supported: ") + extension);
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

  {
    const VkResult result = vkCreateInstance(&ci, nullptr, &instance);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

Status VulkanContext::PickPhysicalDevice(
    const QueueFamilyType queue_family_type) {
  uint32_t device_count = 0;

  {
    const VkResult error =
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if (error != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(error), "Vulkan",
                    error);
    }
  }
  if (device_count == 0) {
    return Status(ErrorCode::kBackend,
                  "failed to find GPUs with Vulkan support!");
  }
  std::vector<VkPhysicalDevice> devices(device_count);

  {
    const VkResult error =
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    if (error != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(error), "Vulkan",
                    error);
    }
  }

  for (const auto& device : devices) {
    {
      const core::Status status = FindQueueFamilies(device, queue_family_type);
      if (!status.ok()) {
        return status;
      }
    }
    if (queue_family_indices_.is_complete() && SupportsDynamicRendering(device)) {
      physical_device = device;
      break;
    }
  }

  if (physical_device == VK_NULL_HANDLE) {
    return Status(ErrorCode::kBackend, "failed to find a suitable GPU!");
  }

  // Get Timestamp period
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(physical_device, &properties);
  const float period = properties.limits.timestampPeriod;
  if (period == 0.0f) {
    return Status(ErrorCode::kUnsupported, "GPU timestamps are unavailable");
  }
  timestamp_period = period;

  return Status();
}

Status VulkanContext::FindQueueFamilies(
    VkPhysicalDevice device, const QueueFamilyType queue_family_type) {
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
      return Status(ErrorCode::kBackend,
                    "Currently only support compute and graphics");
  }

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

  uint32_t i = 0;
  for (const auto& queue_family : queue_families) {
    if (queue_family.queueFlags & compute_flag) {
      queue_family_indices_.compute_family = i;
      break;
    }
    if (queue_family.queueFlags & graphics_flag) {
      queue_family_indices_.graphics_family = i;
      VkBool32 present_support = false;
      const VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(
          device, i, surface_, &present_support);
      if (result != VK_SUCCESS) {
        return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                      result);
      }
      if (present_support) {
        queue_family_indices_.present_family = i;
      }
    }
    if (queue_family_indices_.is_complete()) {
      break;
    }
    ++i;
  }

  return Status();
}

Status VulkanContext::CreateLogicalDevice(const float queuePriority) {
  std::vector<VkDeviceQueueCreateInfo> queue_infos;
  // TODO: support present queue
  std::set<std::optional<uint32_t>> unique_queue_families;
  if (queue_family_indices_.compute_family.has_value()) {
    unique_queue_families.insert(queue_family_indices_.compute_family);
  }
  if (queue_family_indices_.graphics_family.has_value()) {
    unique_queue_families.insert(queue_family_indices_.graphics_family);
  }
  if (queue_family_indices_.present_family.has_value()) {
    unique_queue_families.insert(queue_family_indices_.present_family);
  }

  for (auto family : unique_queue_families) {
    if (!family.has_value()) {
      continue;
    }
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = *family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queuePriority;
    queue_infos.emplace_back(queue_info);
  }

  VkPhysicalDeviceDynamicRenderingFeaturesKHR dyn{};
  dyn.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
  VkPhysicalDeviceFeatures2 features2{};
  features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features2.pNext = &dyn;
  vkGetPhysicalDeviceFeatures2(physical_device, &features2);
  if (dyn.dynamicRendering != VK_TRUE) {
    return Status(ErrorCode::kUnsupported,
                  "Selected GPU does not support dynamic rendering");
  }
  dyn.dynamicRendering = VK_TRUE;  // enable feature at device creation

  // TODO: support more queue family types
  VkDeviceCreateInfo device_create_info{};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pQueueCreateInfos = queue_infos.data();
  device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
  device_create_info.pNext = &dyn;

  const auto extensions_result = GetRequiredDeviceExtensions();
  if (!extensions_result.ok()) {
    return extensions_result.status();
  }
  const auto extensions = *extensions_result;
  device_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  device_create_info.ppEnabledExtensionNames = extensions.data();

  {
    const VkResult result = vkCreateDevice(physical_device, &device_create_info,
                                           nullptr, &logical_device);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  if (queue_family_indices_.compute_family.has_value()) {
    vkGetDeviceQueue(logical_device, *queue_family_indices_.compute_family, 0,
                     &compute_queue_);
  }
  if (queue_family_indices_.graphics_family.has_value()) {
    vkGetDeviceQueue(logical_device, *queue_family_indices_.graphics_family, 0,
                     &graphics_queue_);
  }
  if (queue_family_indices_.present_family.has_value()) {
    vkGetDeviceQueue(logical_device, *queue_family_indices_.present_family, 0,
                     &present_queue_);
  }

  return Status();
}

Result<std::vector<const char*>> VulkanContext::GetRequiredDeviceExtensions()
    const {
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

  // TODO: need to check if dynamic rendering is supported
  extensions.emplace_back("VK_KHR_dynamic_rendering");

  // check if required extensions are supported
  uint32_t extensionCount = 0;

  {
    const VkResult error = vkEnumerateDeviceExtensionProperties(
        physical_device, nullptr, &extensionCount, nullptr);
    if (error != VK_SUCCESS) {
      return Result<std::vector<const char*>>(
          Status(ErrorCode::kBackend, VkErrorMessages(error), "Vulkan", error));
    }
  }
  std::vector<VkExtensionProperties> supportedExtensions(extensionCount);

  {
    const VkResult error = vkEnumerateDeviceExtensionProperties(
        physical_device, nullptr, &extensionCount, supportedExtensions.data());
    if (error != VK_SUCCESS) {
      return Result<std::vector<const char*>>(
          Status(ErrorCode::kBackend, VkErrorMessages(error), "Vulkan", error));
    }
  }

  std::unordered_set<std::string> availableExtensions;
  for (const auto& ext : supportedExtensions) {
    availableExtensions.insert(ext.extensionName);
  }

  for (const auto& ext : extensions) {
    if (availableExtensions.find(ext) == availableExtensions.end()) {
      return Result<std::vector<const char*>>(Status(
          ErrorCode::kUnsupported,
          std::string("Required device extension not supported: ") + ext));
    }
  }

  return Result<std::vector<const char*>>(extensions);
}

Status VulkanContext::SetupDebugMessenger() {
  if (!enable_validation_layers_) return Status();

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
    const VkResult result =
        func(instance, &create_info, nullptr, &debug_messenger_);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

bool VulkanContext::CheckValidationLayerSupport() {
  uint32_t layer_count = 0;
  if (vkEnumerateInstanceLayerProperties(&layer_count, nullptr) != VK_SUCCESS) {
    return false;
  }

  std::vector<VkLayerProperties> available_layers(layer_count);
  if (vkEnumerateInstanceLayerProperties(
          &layer_count, available_layers.data()) != VK_SUCCESS) {
    return false;
  }

  for (const auto& layer : available_layers) {
    if (strcmp(kValidationLayerName[0], layer.layerName) == 0) {
      return true;
    }
  }
  return false;
}

}  // namespace vulkan
}  // namespace core
