#pragma once

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>
#include <string>

#include "Status.h"

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
