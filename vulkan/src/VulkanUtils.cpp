#include "VulkanUtils.h"

#include "VulkanContext.h"

namespace core {
namespace vulkan {

VulkanRenderingCommands LoadDynamicRenderingCommands(VkDevice device) {
  VulkanRenderingCommands cmds{};

  cmds.vkCmdBeginRendering = reinterpret_cast<PFN_vkCmdBeginRendering>(
      vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR"));
  if (!cmds.vkCmdBeginRendering) {
    cmds.vkCmdBeginRendering = reinterpret_cast<PFN_vkCmdBeginRendering>(
        vkGetDeviceProcAddr(device, "vkCmdBeginRendering"));
  }

  cmds.vkCmdEndRendering =
      reinterpret_cast<PFN_vkCmdEndRendering>(vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR"));
  if (!cmds.vkCmdEndRendering) {
    cmds.vkCmdEndRendering =
        reinterpret_cast<PFN_vkCmdEndRendering>(vkGetDeviceProcAddr(device, "vkCmdEndRendering"));
  }

  return cmds;
}

VkSampleCountFlagBits GetMaxUsableSampleCount(const VulkanContext* context) {
  VkPhysicalDeviceProperties physical_device_properties{};
  vkGetPhysicalDeviceProperties(context->physical_device, &physical_device_properties);

  VkSampleCountFlags counts = physical_device_properties.limits.framebufferColorSampleCounts &
                              physical_device_properties.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    return VK_SAMPLE_COUNT_2_BIT;
  }

  return VK_SAMPLE_COUNT_1_BIT;
}

std::string VkErrorMessages(const VkResult result) {
  switch (result) {
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
      return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
      return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
      return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
      return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
      return "VK_ERROR_FEATURE_NOT_PRESENT";
    default:
      return "UNKNOWN";
  }
}

}  // namespace vulkan
}  // namespace core
