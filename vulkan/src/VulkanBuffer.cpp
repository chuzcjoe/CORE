#include "VulkanBuffer.h"

namespace core {
namespace vulkan {

VulkanBuffer::VulkanBuffer(VulkanContext* context, const VkDeviceSize size,
                           const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties)
    : context_(context), buffer_size_(size), memory_properties_(properties) {
  // debug
  // std::cout << "=== Creating VulkanBuffer ===" << std::endl;
  // std::cout << "Size: " << size << std::endl;
  // std::cout << "Usage: " << usage << std::endl;
  // std::cout << "Properties: " << properties << std::endl;
  // std::cout << "Context valid: " << (context_ != nullptr) << std::endl;
  // std::cout << "Logical device: " << (context_->logical_device != VK_NULL_HANDLE) << std::endl;

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

  std::cout << "buffer created successfully, handle: " << buffer << std::endl;
  std::cout << "buffer memory created successfully, handle: " << buffer_memory_ << std::endl;
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
  std::cout << "copying VulkanBuffer\n";
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

void VulkanBuffer::MapData(const std::function<void(void*)>& func) {
  const auto staging_buffer_property =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  if ((memory_properties_ & staging_buffer_property) != staging_buffer_property) {
    throw std::runtime_error("Buffer is not mappable");
  }
  void* data;
  if (!context_->logical_device) {
    throw std::runtime_error("logical deivice is not init");
  }
  vkMapMemory(context_->logical_device, buffer_memory_, 0, buffer_size_, 0, &data);
  func(data);
  vkUnmapMemory(context_->logical_device, buffer_memory_);
}

}  // namespace vulkan
}  // namespace core