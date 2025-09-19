#pragma once

#include "VulkanUtils.h"

namespace core {
namespace vulkan {

class VulkanLoader {
 public:
  VulkanLoader() = default;
  bool Init(PFN_vkGetInstanceProcAddr get_ins_proc_addr = nullptr);
  void LoadInstance(VkInstance instance) { volkLoadInstance(instance); }

  static VulkanLoader& Instance() {
    static VulkanLoader instance;
    return instance;
  }
};

}  // namespace vulkan
}  // namespace core