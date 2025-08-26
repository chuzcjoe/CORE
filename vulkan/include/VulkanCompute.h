#pragma once

#include <vector>

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanPipeline.h"

namespace core {
namespace vulkan {

class VulkanCompute : public VulkanPipeline {
 public:
  explicit VulkanCompute(VulkanContext* context);

 protected:
  void CreatePipeline() override;

 private:
  virtual const std::vector<uint32_t>& LoadShaderCode() const = 0;
};

}  // namespace vulkan
}  // namespace core