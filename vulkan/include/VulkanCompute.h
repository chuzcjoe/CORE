#pragma once

#include <vector>

#include "VulkanBase.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanImage.h"

namespace core {
namespace vulkan {

class VulkanCompute : public VulkanBase {
 public:
  explicit VulkanCompute(VulkanContext* context);

 protected:
  void CreatePipeline() override;

 private:
  virtual const std::vector<uint32_t>& LoadShaderCode() const = 0;
};

}  // namespace vulkan
}  // namespace core