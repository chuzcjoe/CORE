#include "VulkanBuffer.h"

namespace core {
namespace vulkan {

VulkanBuffer::VulkanBuffer(VulkanContext* context, const VkDeviceSize size,
                           const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties)
    : context_(context), buffer_size_(size), memory_properties_(properties) {
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = buffer_size_;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK(vkCreateBuffer(context_->logical_device, &buffer_info, nullptr, &buffer));

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(context_->logical_device, buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      context_->FindMemoryType(mem_requirements.memoryTypeBits, properties);

  VK_CHECK(vkAllocateMemory(context_->logical_device, &alloc_info, nullptr, &buffer_memory_));

  VK_CHECK(vkBindBufferMemory(context_->logical_device, buffer, buffer_memory_, 0));
}

VulkanBuffer::~VulkanBuffer() {
  if (buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(context_->logical_device, buffer, nullptr);
  }
  if (buffer_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(context_->logical_device, buffer_memory_, nullptr);
  }
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& rhs) {
  if (buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(context_->logical_device, buffer, nullptr);
  }
  if (buffer_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(context_->logical_device, buffer_memory_, nullptr);
  }

  context_ = rhs.context_;
  buffer = rhs.buffer;
  rhs.buffer = VK_NULL_HANDLE;

  buffer_memory_ = rhs.buffer_memory_;
  rhs.buffer_memory_ = VK_NULL_HANDLE;

  buffer_size_ = rhs.buffer_size_;
  memory_properties_ = rhs.memory_properties_;

  return *this;
}

// TODO: abstract the copy commands
void VulkanBuffer::CopyBuffer(VulkanBuffer& dst_buffer) {
  VulkanCommandBuffer command_buffer(context_);
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer.buffer(), &begin_info);
  VkBufferCopy copy_region{};
  copy_region.size = buffer_size_;
  vkCmdCopyBuffer(command_buffer.buffer(), buffer, dst_buffer.buffer, 1, &copy_region);
  VkSubmitInfo submit_info{};
  VulkanFence fence(context_);
  fence.Reset();
  command_buffer.Submit(fence.fence, submit_info);
  // Or use vkWaitForFences here
  vkQueueWaitIdle(context_->queue_family_type() == QueueFamilyType::Compute
                      ? context_->compute_queue()
                      : context_->graphics_queue());
}

void VulkanBuffer::MapData(const std::function<void(void*)>& func) {
  const auto staging_buffer_property =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  if ((memory_properties_ & staging_buffer_property) != staging_buffer_property) {
    throw std::runtime_error("Buffer is not mappable");
  }
  void* data;
  vkMapMemory(context_->logical_device, buffer_memory_, 0, buffer_size_, 0, &data);
  func(data);
  vkUnmapMemory(context_->logical_device, buffer_memory_);
}

}  // namespace vulkan
}  // namespace core