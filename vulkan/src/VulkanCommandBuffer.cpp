#include "VulkanCommandBuffer.h"

#include <iostream>

#include "Status.h"

namespace core {
namespace vulkan {

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept
    : context_(other.context_),
      queue_family_type_(other.queue_family_type_),
      command_pool_(other.command_pool_),
      command_buffer_(other.command_buffer_),
      initialized_(other.initialized_) {
  other.context_ = nullptr;
  other.command_pool_ = VK_NULL_HANDLE;
  other.command_buffer_ = VK_NULL_HANDLE;
}

VulkanCommandBuffer& VulkanCommandBuffer::operator=(VulkanCommandBuffer&& other) noexcept {
  if (this == &other) return *this;

  if (context_ && command_buffer_ != VK_NULL_HANDLE && command_pool_ != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(context_->logical_device, command_pool_, 1, &command_buffer_);
  }
  if (context_ && command_pool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(context_->logical_device, command_pool_, nullptr);
  }

  initialized_ = other.initialized_;
  context_ = other.context_;
  queue_family_type_ = other.queue_family_type_;
  command_pool_ = other.command_pool_;
  command_buffer_ = other.command_buffer_;

  other.context_ = nullptr;
  other.command_pool_ = VK_NULL_HANDLE;
  other.command_buffer_ = VK_NULL_HANDLE;

  return *this;
}

Result<std::unique_ptr<VulkanCommandBuffer>> VulkanCommandBuffer::Create(
    VulkanContext* context) {
  std::unique_ptr<VulkanCommandBuffer> object(new VulkanCommandBuffer());
  const Status status = object->Init(context);
  if (!status.ok()) {
    return Result<std::unique_ptr<VulkanCommandBuffer>>(status);
  }
  return Result<std::unique_ptr<VulkanCommandBuffer>>(std::move(object));
}

Status VulkanCommandBuffer::Init(VulkanContext* context) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanCommandBuffer initialization is single-use");
  }
  initialized_ = true;
  context_ = context;
  if (context_ == nullptr || !context_->is_initialized()) {
    return Status(ErrorCode::kInvalidState, "A live Vulkan device is required");
  }

  queue_family_type_ = context_->queue_family_type();

  const QueueFamilyIndices indices = context_->GetQueueFamilyIndices();
  const std::optional<uint32_t> family =
      queue_family_type_ == QueueFamilyType::Compute ? indices.compute_family
                                                     : indices.graphics_family;
  if (!family.has_value()) {
    return Status(ErrorCode::kInvalidState,
                  "Requested queue family is unavailable");
  }
  // Create command pool
  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = *family;
  pool_info.flags =
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;  // Allow resetting individual buffers

  {
    const VkResult result = vkCreateCommandPool(
        context_->logical_device, &pool_info, nullptr, &command_pool_);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  // Allocate command buffer
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool_;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;

  {
    const VkResult result = vkAllocateCommandBuffers(
        context_->logical_device, &alloc_info, &command_buffer_);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
  if (context_ == nullptr || context_->logical_device == VK_NULL_HANDLE) {
    return;
  }

  if (context_ && command_buffer_ != VK_NULL_HANDLE && command_pool_ != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(context_->logical_device, command_pool_, 1, &command_buffer_);
  }
  if (context_ && command_pool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(context_->logical_device, command_pool_, nullptr);
  }
}

Status VulkanCommandBuffer::Submit(const VkFence& fence,
                                   VkSubmitInfo& submit_info) const {
  if (context_ == nullptr || command_buffer_ == VK_NULL_HANDLE ||
      command_pool_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanCommandBuffer requires successful initialization");
  }

  {
    const VkResult result = vkEndCommandBuffer(command_buffer_);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer_;

  VkQueue queue = queue_family_type_ == QueueFamilyType::Compute ? context_->compute_queue()
                                                                 : context_->graphics_queue();
  {
    const VkResult result = vkQueueSubmit(queue, 1, &submit_info, fence);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

Status VulkanCommandBuffer::Submit(const VkFence& fence,
                                   VkSubmitInfo&& submit_info) const {
  if (context_ == nullptr || command_buffer_ == VK_NULL_HANDLE ||
      command_pool_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanCommandBuffer requires successful initialization");
  }

  {
    const core::Status status = Submit(fence, submit_info);
    if (!status.ok()) {
      return status;
    }
  }

  return Status();
}

Status VulkanCommandBuffer::Submit(const VkFence& fence,
                                   const SubmitSyncInfo& sync_info) const {
  if (context_ == nullptr || command_buffer_ == VK_NULL_HANDLE ||
      command_pool_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanCommandBuffer requires successful initialization");
  }

  if (sync_info.wait_semaphores.size() != sync_info.wait_stage_masks.size()) {
    return Status(ErrorCode::kInvalidArgument,
                  "Each wait semaphore requires exactly one stage mask");
  }

  VkSubmitInfo submit_info{};
  submit_info.waitSemaphoreCount = static_cast<uint32_t>(sync_info.wait_semaphores.size());
  submit_info.pWaitSemaphores = sync_info.wait_semaphores.data();
  submit_info.pWaitDstStageMask = sync_info.wait_stage_masks.data();
  submit_info.signalSemaphoreCount = static_cast<uint32_t>(sync_info.signal_semaphores.size());
  submit_info.pSignalSemaphores = sync_info.signal_semaphores.data();
  {
    const core::Status status = Submit(fence, submit_info);
    if (!status.ok()) {
      return status;
    }
  }

  return Status();
}

Status VulkanCommandBuffer::Submit(const VkFence& fence) const {
  if (context_ == nullptr || command_buffer_ == VK_NULL_HANDLE ||
      command_pool_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanCommandBuffer requires successful initialization");
  }

  const VkResult end_result = vkEndCommandBuffer(command_buffer_);
  if (end_result != VK_SUCCESS) {
    return Status(ErrorCode::kBackend, VkErrorMessages(end_result), "Vulkan",
                  end_result);
  }

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer_;

  {
    const VkResult result =
        vkQueueSubmit(queue_family_type_ == QueueFamilyType::Compute
                          ? context_->compute_queue()
                          : context_->graphics_queue(),
                      1, &submit_info, fence);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

Status VulkanCommandBuffer::Reset() {
  if (context_ == nullptr || command_buffer_ == VK_NULL_HANDLE ||
      command_pool_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanCommandBuffer requires successful initialization");
  }
  {
    const VkResult result = vkResetCommandBuffer(command_buffer_, 0);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }
  return Status();
}

Status VulkanCommandBuffer::BeginCommandBuffer() {
  if (context_ == nullptr || command_buffer_ == VK_NULL_HANDLE ||
      command_pool_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanCommandBuffer requires successful initialization");
  }

  const VkCommandBufferBeginInfo begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pInheritanceInfo = nullptr,
  };

  {
    const VkResult result = vkBeginCommandBuffer(command_buffer_, &begin_info);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

Status VulkanCommandBuffer::BeginCommandBufferWithReset() {
  if (context_ == nullptr || command_buffer_ == VK_NULL_HANDLE ||
      command_pool_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanCommandBuffer requires successful initialization");
  }

  {
    const core::Status status = Reset();
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = BeginCommandBuffer();
    if (!status.ok()) {
      return status;
    }
  }

  return Status();
}

Result<std::unique_ptr<VulkanCommandBuffer>>
VulkanCommandBuffer::BeginOneTimeCommands(VulkanContext* context) {
  auto result = Create(context);
  if (!result.ok()) {
    return result;
  }
  const VkCommandBufferBeginInfo begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr,
  };
  const VkResult error = vkBeginCommandBuffer((*result)->buffer(), &begin_info);
  if (error != VK_SUCCESS) {
    return Result<std::unique_ptr<VulkanCommandBuffer>>(
        Status(ErrorCode::kBackend, VkErrorMessages(error), "Vulkan", error));
  }
  return result;
}

Status VulkanCommandBuffer::EndOneTimeCommands() const {
  if (context_ == nullptr || command_buffer_ == VK_NULL_HANDLE ||
      command_pool_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanCommandBuffer requires successful initialization");
  }

  auto fence_result = VulkanFence::Create(context_);
  if (!fence_result.ok()) {
    return fence_result.status();
  }
  std::unique_ptr<VulkanFence> fence_owner = std::move(*fence_result);
  VulkanFence& fence = *fence_owner;
  {
    const core::Status status = fence.Reset();
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = Submit(fence.fence);
    if (!status.ok()) {
      return status;
    }
  }
  const VkResult wait_result = vkWaitForFences(
      context_->logical_device, 1, &fence.fence, VK_TRUE, UINT64_MAX);
  if (wait_result != VK_SUCCESS) {
    return Status(ErrorCode::kBackend, VkErrorMessages(wait_result), "Vulkan",
                  wait_result);
  }

  return Status();
}

}  // namespace vulkan
}  // namespace core
