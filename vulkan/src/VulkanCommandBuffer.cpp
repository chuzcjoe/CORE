#include "VulkanCommandBuffer.h"

#include <iostream>

namespace core {
namespace vulkan {

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept
    : context_(other.context_),
      queue_family_type_(other.queue_family_type_),
      command_pool_(other.command_pool_),
      command_buffer_(other.command_buffer_) {
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

  context_ = other.context_;
  queue_family_type_ = other.queue_family_type_;
  command_pool_ = other.command_pool_;
  command_buffer_ = other.command_buffer_;

  other.context_ = nullptr;
  other.command_pool_ = VK_NULL_HANDLE;
  other.command_buffer_ = VK_NULL_HANDLE;

  return *this;
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanContext* context)
    : context_(context), queue_family_type_(context_->queue_family_type()) {
  // Create command pool
  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = queue_family_type_ == QueueFamilyType::Compute
                                   ? context_->GetQueueFamilyIndices().compute_family.value()
                                   : context_->GetQueueFamilyIndices().graphics_family.value();
  pool_info.flags =
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;  // Allow resetting individual buffers

  VK_CHECK(vkCreateCommandPool(context_->logical_device, &pool_info, nullptr, &command_pool_));

  // Allocate command buffer
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool_;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;

  VK_CHECK(vkAllocateCommandBuffers(context_->logical_device, &alloc_info, &command_buffer_));
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
  if (context_ && command_buffer_ != VK_NULL_HANDLE && command_pool_ != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(context_->logical_device, command_pool_, 1, &command_buffer_);
  }
  if (context_ && command_pool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(context_->logical_device, command_pool_, nullptr);
  }
}

void VulkanCommandBuffer::Submit(const VkFence& fence, VkSubmitInfo& submit_info) const {
  std::cout << "Submit: before vkEndCommandBuffer" << std::endl;
  VK_CHECK(vkEndCommandBuffer(command_buffer_));
  std::cout << "Submit: after vkEndCommandBuffer" << std::endl;

  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer_;

  VkQueue queue = queue_family_type_ == QueueFamilyType::Compute ? context_->compute_queue()
                                                                 : context_->graphics_queue();
  std::cout << "Submit: using queue = " << queue
            << ", waitSemaphoreCount = " << submit_info.waitSemaphoreCount
            << ", signalSemaphoreCount = " << submit_info.signalSemaphoreCount << std::endl;
  std::cout << "Submit: before vkQueueSubmit" << std::endl;
  VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, fence));
  std::cout << "Submit: after vkQueueSubmit" << std::endl;
}

void VulkanCommandBuffer::Submit(const VkFence& fence) const {
  vkEndCommandBuffer(command_buffer_);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer_;

  VK_CHECK(vkQueueSubmit(queue_family_type_ == QueueFamilyType::Compute
                             ? context_->compute_queue()
                             : context_->graphics_queue(),
                         1, &submit_info, fence));
}

void VulkanCommandBuffer::Reset() { VK_CHECK(vkResetCommandBuffer(command_buffer_, 0)); }

VulkanCommandBuffer VulkanCommandBuffer::BeginOneTimeCommands(VulkanContext* context) {
  VulkanCommandBuffer command_buffer(context);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VK_CHECK(vkBeginCommandBuffer(command_buffer.buffer(), &begin_info));
  return command_buffer;
}

void VulkanCommandBuffer::EndOneTimeCommands() const {
  VulkanFence fence(context_);
  fence.Reset();
  Submit(fence.fence);
  vkWaitForFences(context_->logical_device, 1, &fence.fence, VK_TRUE, UINT64_MAX);
}

}  // namespace vulkan
}  // namespace core
