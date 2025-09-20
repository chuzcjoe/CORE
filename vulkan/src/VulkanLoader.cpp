#include "VulkanLoader.h"

namespace core {
namespace vulkan {

bool VulkanLoader::Init(PFN_vkGetInstanceProcAddr get_ins_proc_addr) {
  if (get_ins_proc_addr) {
    volkInitializeCustom(get_ins_proc_addr);
    return true;
  }

#if defined(__APPLE__)
  vulkan_rt_ = std::make_unique<DynamicLoader>(runtime_lib_dir() + "/libMoltenVK.dylib");
  PFN_vkGetInstanceProcAddr get_proc_addr =
      (PFN_vkGetInstanceProcAddr)vulkan_rt_->load_function("vkGetInstanceProcAddr");

  volkInitializeCustom(get_proc_addr);
  initialized_ = true;
#else
  VkResult result = volkInitialize();
  return result == VK_SUCCESS;
#endif
}

}  // namespace vulkan
}  // namespace core