#include "VulkanBuffer.h"

#include <vector>

#include "Status.h"
#include "VulkanImage.h"

namespace core {
namespace vulkan {

Result<std::unique_ptr<VulkanBuffer>> VulkanBuffer::Create(
    VulkanContext* context, const VkDeviceSize size,
    const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties) {
  std::unique_ptr<VulkanBuffer> object(new VulkanBuffer());
  const Status status = object->Init(context, size, usage, properties);
  if (!status.ok()) {
    return Result<std::unique_ptr<VulkanBuffer>>(status);
  }
  return Result<std::unique_ptr<VulkanBuffer>>(std::move(object));
}

Status VulkanBuffer::Init(VulkanContext* context, const VkDeviceSize size,
                          const VkBufferUsageFlags usage,
                          const VkMemoryPropertyFlags properties) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanBuffer initialization is single-use");
  }
  initialized_ = true;
  context_ = context;
  if (context_ == nullptr || !context_->is_initialized()) {
    return Status(ErrorCode::kInvalidState, "A live Vulkan device is required");
  }

  buffer_size_ = size;
  memory_properties_ = properties;
  usage_ = usage;

  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = buffer_size_;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  {
    const VkResult result = vkCreateBuffer(context_->logical_device,
                                           &buffer_info, nullptr, &buffer);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(context_->logical_device, buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;

  const auto alloc_info_memoryTypeIndex_result =
      context_->FindMemoryType(mem_requirements.memoryTypeBits, properties);
  if (!alloc_info_memoryTypeIndex_result.ok()) {
    return alloc_info_memoryTypeIndex_result.status();
  }
  alloc_info.memoryTypeIndex = *alloc_info_memoryTypeIndex_result;

  {
    const VkResult result = vkAllocateMemory(
        context_->logical_device, &alloc_info, nullptr, &buffer_memory_);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  {
    const VkResult result =
        vkBindBufferMemory(context_->logical_device, buffer, buffer_memory_, 0);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

VulkanBuffer::~VulkanBuffer() {
  if (context_ == nullptr || context_->logical_device == VK_NULL_HANDLE) {
    return;
  }

  if (context_ && buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(context_->logical_device, buffer, nullptr);
  }
  if (context_ && buffer_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(context_->logical_device, buffer_memory_, nullptr);
  }
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& rhs) {
  if (this == &rhs) {
    return *this;
  }

  if (context_ && buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(context_->logical_device, buffer, nullptr);
  }
  if (context_ && buffer_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(context_->logical_device, buffer_memory_, nullptr);
  }

  initialized_ = rhs.initialized_;
  context_ = rhs.context_;
  buffer = rhs.buffer;
  rhs.buffer = VK_NULL_HANDLE;

  buffer_memory_ = rhs.buffer_memory_;
  rhs.buffer_memory_ = VK_NULL_HANDLE;

  buffer_size_ = rhs.buffer_size_;
  memory_properties_ = rhs.memory_properties_;
  usage_ = rhs.usage_;

  return *this;
}

Status VulkanBuffer::CopyToBuffer(VulkanBuffer& dst_buffer) {
  if (context_ == nullptr || buffer == VK_NULL_HANDLE ||
      buffer_memory_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanBuffer requires successful initialization");
  }

  if (buffer_size_ != dst_buffer.Size()) {
    return Status(ErrorCode::kBackend, "Buffer sizes do not match for copy");
  }

  auto command_buffer_result =
      VulkanCommandBuffer::BeginOneTimeCommands(context_);
  if (!command_buffer_result.ok()) {
    return command_buffer_result.status();
  }
  auto command_buffer_owner = std::move(*command_buffer_result);
  auto& command_buffer = *command_buffer_owner;
  VkBufferCopy copy_region{};
  copy_region.size = buffer_size_;
  vkCmdCopyBuffer(command_buffer.buffer(), buffer, dst_buffer.buffer, 1, &copy_region);
  {
    const core::Status status = command_buffer.EndOneTimeCommands();
    if (!status.ok()) {
      return status;
    }
  }

  return Status();
}

Status VulkanBuffer::CopyBuffers(std::span<const BatchBufferCopy> copies) {
  if (copies.empty()) {
    return Status();
  }

  VulkanContext* context = nullptr;
  std::vector<VkBufferCopy> regions;
  regions.reserve(copies.size());
  for (const BatchBufferCopy& copy : copies) {
    if (copy.source == nullptr || copy.destination == nullptr) {
      return Status(ErrorCode::kInvalidArgument,
                    "Buffer copy requires source and destination");
    }
    const VulkanBuffer& source = *copy.source;
    const VulkanBuffer& destination = *copy.destination;
    if (context == nullptr) {
      context = source.context_;
    }
    if (context == nullptr || source.context_ != context || destination.context_ != context ||
        source.buffer == VK_NULL_HANDLE || destination.buffer == VK_NULL_HANDLE) {
      return Status(ErrorCode::kInvalidArgument,
                    "Buffer copies require live buffers from one context");
    }
    if ((source.usage_ & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0 ||
        (destination.usage_ & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0) {
      return Status(ErrorCode::kInvalidArgument,
                    "Buffer copy requires transfer usage flags");
    }
    if (copy.source_offset >= source.buffer_size_ ||
        copy.destination_offset >= destination.buffer_size_) {
      return Status(ErrorCode::kOutOfRange,
                    "Buffer copy offset is outside the buffer");
    }
    const VkDeviceSize size = copy.size == VK_WHOLE_SIZE
                                  ? source.buffer_size_ - copy.source_offset
                                  : copy.size;
    if (size == 0 || size > source.buffer_size_ - copy.source_offset ||
        size > destination.buffer_size_ - copy.destination_offset) {
      return Status(ErrorCode::kOutOfRange,
                    "Buffer copy size is outside the buffer");
    }
    if (source.buffer == destination.buffer &&
        copy.source_offset < copy.destination_offset + size &&
        copy.destination_offset < copy.source_offset + size) {
      return Status(ErrorCode::kInvalidArgument,
                    "Buffer copy source and destination overlap");
    }
    regions.push_back(VkBufferCopy{
        .srcOffset = copy.source_offset,
        .dstOffset = copy.destination_offset,
        .size = size,
    });
  }

  auto command_buffer_result =
      VulkanCommandBuffer::BeginOneTimeCommands(context);
  if (!command_buffer_result.ok()) {
    return command_buffer_result.status();
  }
  auto command_buffer_owner = std::move(*command_buffer_result);
  auto& command_buffer = *command_buffer_owner;
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

  auto fence_result = VulkanFence::Create(context);
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
    const core::Status status = command_buffer.Submit(fence.fence);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const VkResult result = vkWaitForFences(context->logical_device, 1,
                                            &fence.fence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

Status VulkanBuffer::CopyToImage(VulkanImage& dst_image, const uint32_t width,
                                 const uint32_t height, const uint32_t layers) {
  if (context_ == nullptr || buffer == VK_NULL_HANDLE ||
      buffer_memory_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanBuffer requires successful initialization");
  }

  auto command_buffer_result =
      VulkanCommandBuffer::BeginOneTimeCommands(context_);
  if (!command_buffer_result.ok()) {
    return command_buffer_result.status();
  }
  auto command_buffer_owner = std::move(*command_buffer_result);
  auto& command_buffer = *command_buffer_owner;

  std::vector<VkBufferImageCopy> regions(layers);
  VkDeviceSize layer_size = buffer_size_ / layers;

  for (uint32_t i = 0; i < layers; ++i) {
    VkBufferImageCopy region{};
    region.bufferOffset = layer_size * i;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = i;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    regions[i] = region;
  }

  vkCmdCopyBufferToImage(command_buffer.buffer(), buffer, dst_image.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());

  {
    const core::Status status = command_buffer.EndOneTimeCommands();
    if (!status.ok()) {
      return status;
    }
  }

  return Status();
}

Status VulkanBuffer::MapData(const std::function<void(void*)>& func) {
  if (context_ == nullptr || buffer == VK_NULL_HANDLE ||
      buffer_memory_ == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanBuffer requires successful initialization");
  }

  const auto staging_buffer_property =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  if ((memory_properties_ & staging_buffer_property) != staging_buffer_property) {
    return Status(ErrorCode::kBackend, "Buffer is not mappable");
  }
  if (context_ == nullptr || buffer == VK_NULL_HANDLE || !func) {
    return Status(ErrorCode::kInvalidState,
                  "Mapping requires a live buffer and callback");
  }
  void* data = nullptr;
  const VkResult result = vkMapMemory(context_->logical_device, buffer_memory_,
                                      0, buffer_size_, 0, &data);
  if (result != VK_SUCCESS) {
    return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                  result);
  }
  func(data);
  vkUnmapMemory(context_->logical_device, buffer_memory_);

  return Status();
}

}  // namespace vulkan
}  // namespace core
