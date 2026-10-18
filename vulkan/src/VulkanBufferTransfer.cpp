#include "VulkanBuffer.h"

#include <stdexcept>
#include <vector>

namespace core {
namespace vulkan {

void VulkanBuffer::CopyBuffers(std::span<const BufferCopy> copies) {
  if (copies.empty()) {
    return;
  }

  VulkanContext* context = nullptr;
  std::vector<VkBufferCopy> regions;
  regions.reserve(copies.size());
  for (const BufferCopy& copy : copies) {
    if (copy.source == nullptr || copy.destination == nullptr) {
      throw std::invalid_argument("Buffer copy requires source and destination");
    }
    const VulkanBuffer& source = *copy.source;
    const VulkanBuffer& destination = *copy.destination;
    if (context == nullptr) {
      context = source.context_;
    }
    if (context == nullptr || source.context_ != context || destination.context_ != context ||
        source.buffer == VK_NULL_HANDLE || destination.buffer == VK_NULL_HANDLE) {
      throw std::invalid_argument("Buffer copies require live buffers from one context");
    }
    if ((source.usage_ & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0 ||
        (destination.usage_ & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0) {
      throw std::invalid_argument("Buffer copy requires transfer usage flags");
    }
    if (copy.source_offset >= source.buffer_size_ ||
        copy.destination_offset >= destination.buffer_size_) {
      throw std::out_of_range("Buffer copy offset is outside the buffer");
    }
    const VkDeviceSize size = copy.size == VK_WHOLE_SIZE
                                  ? source.buffer_size_ - copy.source_offset
                                  : copy.size;
    if (size == 0 || size > source.buffer_size_ - copy.source_offset ||
        size > destination.buffer_size_ - copy.destination_offset) {
      throw std::out_of_range("Buffer copy size is outside the buffer");
    }
    if (source.buffer == destination.buffer &&
        copy.source_offset < copy.destination_offset + size &&
        copy.destination_offset < copy.source_offset + size) {
      throw std::invalid_argument("Buffer copy source and destination overlap");
    }
    regions.push_back(VkBufferCopy{
        .srcOffset = copy.source_offset,
        .dstOffset = copy.destination_offset,
        .size = size,
    });
  }

  const VulkanCommandBuffer command_buffer = VulkanCommandBuffer::BeginOneTimeCommands(context);
  const VkMemoryBarrier before{
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
  };
  vkCmdPipelineBarrier(command_buffer.buffer(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &before, 0, nullptr, 0, nullptr);

  const VkMemoryBarrier between{
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
  };
  for (size_t index = 0; index < copies.size(); ++index) {
    if (index != 0) {
      // Preserve array order, including read-after-write and overlapping writes.
      vkCmdPipelineBarrier(command_buffer.buffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &between, 0, nullptr, 0, nullptr);
    }
    vkCmdCopyBuffer(command_buffer.buffer(), copies[index].source->buffer,
                    copies[index].destination->buffer, 1, &regions[index]);
  }

  const VkMemoryBarrier after{
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT |
                       VK_ACCESS_HOST_READ_BIT,
  };
  vkCmdPipelineBarrier(command_buffer.buffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_HOST_BIT, 0, 1,
                       &after, 0, nullptr, 0, nullptr);

  VulkanFence fence(context);
  fence.Reset();
  command_buffer.Submit(fence.fence);
  VK_CHECK(vkWaitForFences(context->logical_device, 1, &fence.fence, VK_TRUE, UINT64_MAX));
}

}  // namespace vulkan
}  // namespace core
