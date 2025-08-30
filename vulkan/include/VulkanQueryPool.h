#pragma once

#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanQueryPool {
 public:
  VulkanQueryPool(VulkanContext* context, const VkQueryType query_type);
  ~VulkanQueryPool();

  void Reset(const VkCommandBuffer command);
  void Query(const VkCommandBuffer command, uint32_t query,
             VkPipelineStageFlagBits stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
  void GetQueryResults();

  std::vector<uint64_t> GetTimeStamps() const { return timestamps_; }

 private:
  VulkanContext* context_;
  VkQueryType query_type_;
  VkQueryPool query_pool_;

  std::vector<uint64_t> timestamps_;
};
}  // namespace vulkan
}  // namespace core