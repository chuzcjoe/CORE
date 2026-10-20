#pragma once

#include <memory>

#include "Status.h"
#include "VulkanContext.h"

namespace core {
namespace vulkan {

class VulkanQueryPool {
 public:
  VulkanQueryPool(const VulkanQueryPool&) = delete;
  VulkanQueryPool& operator=(const VulkanQueryPool&) = delete;
  VulkanQueryPool() = default;

  static Result<std::unique_ptr<VulkanQueryPool>> Create(
      VulkanContext* context, const VkQueryType query_type);
  Status Init(VulkanContext* context, const VkQueryType query_type);
  ~VulkanQueryPool();

  Status Reset(const VkCommandBuffer command);
  Status Query(
      const VkCommandBuffer command, uint32_t query,
      VkPipelineStageFlagBits stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
  Status GetQueryResults();

  std::vector<uint64_t> GetTimeStamps() const { return timestamps_; }

 private:
  VulkanContext* context_ = nullptr;
  VkQueryType query_type_;
  VkQueryPool query_pool_ = VK_NULL_HANDLE;

  std::vector<uint64_t> timestamps_;

 private:
  bool initialized_ = false;
};
}  // namespace vulkan
}  // namespace core
