#include "VulkanLoader.h"

namespace core {
namespace vulkan {

bool VulkanLoader::Init(PFN_vkGetInstanceProcAddr get_ins_proc_addr) {
  if (get_ins_proc_addr) {
    volkInitializeCustom(get_ins_proc_addr);
    return true;
  }

  VkResult result = volkInitialize();
  return result == VK_SUCCESS;
}

}  // namespace vulkan
}  // namespace core