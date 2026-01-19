#pragma once

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>
#include <stdexcept>
#include <string>

#define VK_CHECK(x)                                                 \
  do {                                                              \
    VkResult err = x;                                               \
    if (err != VK_SUCCESS) {                                        \
      printf("Detected Vulkan error: %d\n", err);                   \
      throw std::runtime_error(core::vulkan::VkErrorMessages(err)); \
    }                                                               \
  } while (0)

namespace core {
namespace vulkan {

class VulkanContext;

std::string VkErrorMessages(const VkResult result);

enum class QueueFamilyType { Compute, Graphics };

struct VulkanRenderingCommands {
  PFN_vkCmdBeginRendering vkCmdBeginRendering = nullptr;
  PFN_vkCmdEndRendering vkCmdEndRendering = nullptr;
};

VulkanRenderingCommands LoadDynamicRenderingCommands(VkDevice device);

VkSampleCountFlagBits GetMaxUsableSampleCount(const VulkanContext* context);

}  // namespace vulkan
}  // namespace core
