#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include "Status.h"
#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanSampler {
 public:
  VulkanSampler(const VulkanSampler&) = delete;
  VulkanSampler& operator=(const VulkanSampler&) = delete;
  VulkanSampler() = default;

  static Result<std::unique_ptr<VulkanSampler>> Create(VulkanContext* context);
  Status Init(VulkanContext* context);
  ~VulkanSampler();

 private:
  VulkanContext* context_ = nullptr;

 public:
  VkSampler sampler = VK_NULL_HANDLE;

 private:
  bool initialized_ = false;
};

}  // namespace vulkan
}  // namespace core
