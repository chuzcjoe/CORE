#include "VulkanQueryPool.h"

namespace core {
namespace vulkan {

VulkanQueryPool::VulkanQueryPool(VulkanContext* context, const VkQueryType query_type)
    : context_(context), query_type_(query_type), timestamps_(2) {
  VkQueryPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  pool_info.queryType = query_type_;
  if (query_type_ == VK_QUERY_TYPE_TIMESTAMP) {
    pool_info.queryCount = 2;
  } else {
    std::runtime_error("Query type is not supported");
  }
  VK_CHECK(vkCreateQueryPool(context_->logical_device, &pool_info, nullptr, &query_pool_));
}

VulkanQueryPool::~VulkanQueryPool() {
  vkDestroyQueryPool(context_->logical_device, query_pool_, nullptr);
}

void VulkanQueryPool::Reset(const VkCommandBuffer command) {
  if (query_type_ == VK_QUERY_TYPE_TIMESTAMP) {
    vkCmdResetQueryPool(command, query_pool_, 0, timestamps_.size());
  } else {
    std::runtime_error("Query type is not supported");
  }
}

void VulkanQueryPool::Query(const VkCommandBuffer command, uint32_t query,
                            VkPipelineStageFlagBits stage) {
  if (query_type_ == VK_QUERY_TYPE_TIMESTAMP) {
    vkCmdWriteTimestamp(command, stage, query_pool_, query);
  } else {
    std::runtime_error("Query type is not supported");
  }
}

void VulkanQueryPool::GetQueryResults() {
  if (query_type_ == VK_QUERY_TYPE_TIMESTAMP) {
    vkGetQueryPoolResults(context_->logical_device, query_pool_, 0, timestamps_.size(),
                          timestamps_.size() * sizeof(uint64_t), timestamps_.data(),
                          sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
  } else {
    std::runtime_error("Query type is not supported");
  }
}

}  // namespace vulkan
}  // namespace core