#pragma once

namespace core {
namespace vulkan {

#define VK_CHECK(x)                               \
  do {                                            \
    VkResult err = x;                             \
    if (err != VK_SUCCESS) {                      \
      printf("Detected Vulkan error: %d\n", err); \
      abort();                                    \
    }                                             \
  } while (0)

enum class QueueFamilyType {
  Compute,
  Graphics,
  Present,
  ComputeAndGraphics,
  GraphicsAndPresent,
  All
};

}  // namespace vulkan
}  // namespace core