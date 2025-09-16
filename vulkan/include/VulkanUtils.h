#pragma once

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>
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

std::string VkErrorMessages(const VkResult result);

enum class QueueFamilyType { Compute, Graphics };

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
};

}  // namespace vulkan
}  // namespace core