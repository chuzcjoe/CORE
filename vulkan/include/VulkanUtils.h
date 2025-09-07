#pragma once

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>
#include <string>

namespace core {
namespace vulkan {

#define VK_CHECK(x)                               \
  do {                                            \
    VkResult err = x;                             \
    if (err != VK_SUCCESS) {                      \
      printf("Detected Vulkan error: %d\n", err); \
      std::runtime_error(VkErrorMessages(err));   \
    }                                             \
  } while (0)

std::string VkErrorMessages(const VkResult result);

enum class QueueFamilyType {
  Compute,
  Graphics,
  Present,
  ComputeAndGraphics,
  GraphicsAndPresent,
  All
};

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
};

}  // namespace vulkan
}  // namespace core