#include "VulkanBuffer.h"

#include "VulkanImage.h"

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

void VulkanBuffer::CopyToBuffer(VulkanBuffer& dst_buffer) {
  if (buffer_size_ != dst_buffer.Size()) {
    throw std::runtime_error("Buffer sizes do not match for copy");
  }

  const auto command_buffer = VulkanCommandBuffer::BeginOneTimeCommands(context_);
  VkBufferCopy copy_region{};
  copy_region.size = buffer_size_;
  vkCmdCopyBuffer(command_buffer.buffer(), buffer, dst_buffer.buffer, 1, &copy_region);
  command_buffer.EndOneTimeCommands();
}

void VulkanBuffer::CopyToImage(VulkanImage& dst_image, const uint32_t width,
                               const uint32_t height) {
  const auto command_buffer = VulkanCommandBuffer::BeginOneTimeCommands(context_);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(command_buffer.buffer(), buffer, dst_image.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  command_buffer.EndOneTimeCommands();
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