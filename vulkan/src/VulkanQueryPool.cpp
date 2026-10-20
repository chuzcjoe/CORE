#include "VulkanQueryPool.h"

namespace core {
namespace vulkan {

Result<std::unique_ptr<VulkanQueryPool>> VulkanQueryPool::Create(
    VulkanContext* context, const VkQueryType query_type) {
  std::unique_ptr<VulkanQueryPool> object(new VulkanQueryPool());
  const Status status = object->Init(context, query_type);
  if (!status.ok()) {
    return Result<std::unique_ptr<VulkanQueryPool>>(status);
  }
  return Result<std::unique_ptr<VulkanQueryPool>>(std::move(object));
}

Status VulkanQueryPool::Init(VulkanContext* context,
                             const VkQueryType query_type) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanQueryPool initialization is single-use");
  }
  initialized_ = true;
  context_ = context;
  if (context_ == nullptr || !context_->is_initialized()) {
    return Status(ErrorCode::kInvalidState, "A live Vulkan device is required");
  }

  query_type_ = query_type;
  timestamps_.resize(2);

  VkQueryPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  pool_info.queryType = query_type_;
  if (query_type_ == VK_QUERY_TYPE_TIMESTAMP) {
    pool_info.queryCount = 2;
  } else {
    return Status(ErrorCode::kUnsupported, "Query type is not supported");
  }
  {
    const VkResult result = vkCreateQueryPool(
        context_->logical_device, &pool_info, nullptr, &query_pool_);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

VulkanQueryPool::~VulkanQueryPool() {
  if (context_ == nullptr || context_->logical_device == VK_NULL_HANDLE) {
    return;
  }

  vkDestroyQueryPool(context_->logical_device, query_pool_, nullptr);
}

Status VulkanQueryPool::Reset(const VkCommandBuffer command) {
  if (context_ == nullptr || query_pool_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanQueryPool requires successful initialization");
  }

  if (query_type_ == VK_QUERY_TYPE_TIMESTAMP) {
    vkCmdResetQueryPool(command, query_pool_, 0, timestamps_.size());
  } else {
    return Status(ErrorCode::kUnsupported, "Query type is not supported");
  }

  return Status();
}

Status VulkanQueryPool::Query(const VkCommandBuffer command, uint32_t query,
                              VkPipelineStageFlagBits stage) {
  if (context_ == nullptr || query_pool_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanQueryPool requires successful initialization");
  }

  if (query >= timestamps_.size()) {
    return Status(ErrorCode::kOutOfRange, "Invalid timestamp index");
  }
  if (query_type_ == VK_QUERY_TYPE_TIMESTAMP) {
    vkCmdWriteTimestamp(command, stage, query_pool_, query);
  } else {
    return Status(ErrorCode::kUnsupported, "Query type is not supported");
  }

  return Status();
}

Status VulkanQueryPool::GetQueryResults() {
  if (context_ == nullptr || query_pool_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanQueryPool requires successful initialization");
  }

  if (query_type_ == VK_QUERY_TYPE_TIMESTAMP) {
    const VkResult result = vkGetQueryPoolResults(
        context_->logical_device, query_pool_, 0, timestamps_.size(),
        timestamps_.size() * sizeof(uint64_t), timestamps_.data(),
        sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  } else {
    return Status(ErrorCode::kUnsupported, "Query type is not supported");
  }

  return Status();
}

}  // namespace vulkan
}  // namespace core
