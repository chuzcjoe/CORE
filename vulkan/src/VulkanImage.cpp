#include "VulkanImage.h"

#include "Status.h"
#include "VulkanBuffer.h"

namespace core {
namespace vulkan {

Result<std::unique_ptr<VulkanImage>> VulkanImage::Create(
    VulkanContext* context, const uint32_t width, const uint32_t height,
    const VkFormat format, const VkImageUsageFlags usage,
    const VkImageAspectFlags aspect, const VkMemoryPropertyFlags properties,
    const VkImageTiling tiling, const uint32_t mip_levels,
    const VkSampleCountFlagBits samples, const VkImageCreateFlags flags,
    const uint32_t layers) {
  std::unique_ptr<VulkanImage> object(new VulkanImage());
  const Status status =
      object->Init(context, width, height, format, usage, aspect, properties,
                   tiling, mip_levels, samples, flags, layers);
  if (!status.ok()) {
    return Result<std::unique_ptr<VulkanImage>>(status);
  }
  return Result<std::unique_ptr<VulkanImage>>(std::move(object));
}

Status VulkanImage::Init(VulkanContext* context, const uint32_t width,
                         const uint32_t height, const VkFormat format,
                         const VkImageUsageFlags usage,
                         const VkImageAspectFlags aspect,
                         const VkMemoryPropertyFlags properties,
                         const VkImageTiling tiling, const uint32_t mip_levels,
                         const VkSampleCountFlagBits samples,
                         const VkImageCreateFlags flags,
                         const uint32_t layers) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanImage initialization is single-use");
  }
  initialized_ = true;
  context_ = context;
  if (context_ == nullptr || !context_->is_initialized()) {
    return Status(ErrorCode::kInvalidState, "A live Vulkan device is required");
  }

  image_format_ = format;
  mip_levels_ = mip_levels;
  array_layers_ = layers;
  samples_ = samples;
  image_width = width;
  image_height = height;

  VkImageCreateInfo image_info{};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = image_width;
  image_info.extent.height = image_height;
  image_info.extent.depth = 1;  // 2D image has depth 1
  image_info.mipLevels = mip_levels_;
  image_info.arrayLayers = layers;
  image_info.format = image_format_;
  image_info.tiling = tiling;
  image_info.initialLayout =
      VK_IMAGE_LAYOUT_UNDEFINED;  // only two states: UNDEFINED and PREINITIALIZED
  image_info.usage = usage;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.samples = samples_;
  image_info.flags = flags;  // Optional

  {
    const VkResult result =
        vkCreateImage(context_->logical_device, &image_info, nullptr, &image);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(context_->logical_device, image, &mem_requirements);

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
        context_->logical_device, &alloc_info, nullptr, &image_memory);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  const VkResult bind_result =
      vkBindImageMemory(context_->logical_device, image, image_memory, 0);
  if (bind_result != VK_SUCCESS) {
    return Status(ErrorCode::kBackend, VkErrorMessages(bind_result), "Vulkan",
                  bind_result);
  }

  // Create image view
  VkImageViewCreateInfo view_info{};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image;
  if ((flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) != 0 && array_layers_ >= 6) {
    view_info.viewType =
        array_layers_ == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
  } else if (array_layers_ > 1) {
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
  } else {
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  }
  view_info.format = image_format_;
  view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.subresourceRange.aspectMask = aspect;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = mip_levels_;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = layers;

  {
    const VkResult result = vkCreateImageView(context_->logical_device,
                                              &view_info, nullptr, &image_view);
    if (result != VK_SUCCESS) {
      return Status(ErrorCode::kBackend, VkErrorMessages(result), "Vulkan",
                    result);
    }
  }

  return Status();
}

VulkanImage::~VulkanImage() {
  if (context_ == nullptr || context_->logical_device == VK_NULL_HANDLE) {
    return;
  }

  if (context_ && image_view != VK_NULL_HANDLE) {
    vkDestroyImageView(context_->logical_device, image_view, nullptr);
  }
  if (context_ && image != VK_NULL_HANDLE) {
    vkDestroyImage(context_->logical_device, image, nullptr);
  }
  if (context_ && image_memory != VK_NULL_HANDLE) {
    vkFreeMemory(context_->logical_device, image_memory, nullptr);
  }
}

VulkanImage& VulkanImage::operator=(VulkanImage&& rhs) {
  if (this == &rhs) {
    return *this;
  }

  if (context_ && image_view != VK_NULL_HANDLE) {
    vkDestroyImageView(context_->logical_device, image_view, nullptr);
  }
  if (context_ && image != VK_NULL_HANDLE) {
    vkDestroyImage(context_->logical_device, image, nullptr);
  }
  if (context_ && image_memory != VK_NULL_HANDLE) {
    vkFreeMemory(context_->logical_device, image_memory, nullptr);
  }

  initialized_ = rhs.initialized_;
  context_ = rhs.context_;
  image_format_ = rhs.image_format_;
  mip_levels_ = rhs.mip_levels_;
  array_layers_ = rhs.array_layers_;
  samples_ = rhs.samples_;

  image = rhs.image;
  rhs.image = VK_NULL_HANDLE;

  image_view = rhs.image_view;
  rhs.image_view = VK_NULL_HANDLE;

  image_memory = rhs.image_memory;
  rhs.image_memory = VK_NULL_HANDLE;

  image_width = rhs.image_width;
  image_height = rhs.image_height;

  return *this;
}

Status VulkanImage::TransitionImageLayout(
    const VkImageLayout old_layout, const VkImageLayout new_layout,
    [[maybe_unused]] const VkFormat format, const uint32_t mip_levels,
    const uint32_t layers) {
  if (context_ == nullptr || image == VK_NULL_HANDLE ||
      image_view == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanImage requires successful initialization");
  }

  auto command_buffer_result =
      VulkanCommandBuffer::BeginOneTimeCommands(context_);
  if (!command_buffer_result.ok()) {
    return command_buffer_result.status();
  }
  auto command_buffer_owner = std::move(*command_buffer_result);
  auto& command_buffer = *command_buffer_owner;

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = mip_levels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = layers;

  VkPipelineStageFlags source_stage;
  VkPipelineStageFlags destination_stage;

  /**
   * case 1: undefined -> transfer destination
   * case 2: transfer destination -> shader read
   */
  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;  // Not a real stage within the graphics and
                                                         // compute pipelines. It is more of a
                                                         // pseudo-stage where transfers happen
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;

    source_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    destination_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  } else {
    return Status(ErrorCode::kInvalidArgument,
                  "unsupported layout transition!");
  }

  vkCmdPipelineBarrier(command_buffer.buffer(), source_stage, destination_stage, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  {
    const core::Status status = command_buffer.EndOneTimeCommands();
    if (!status.ok()) {
      return status;
    }
  }

  return Status();
}

Status VulkanImage::TransitionDepthImageLayout(
    const VkImageLayout old_layout, const VkImageLayout new_layout,
    [[maybe_unused]] const VkFormat format) {
  if (context_ == nullptr || image == VK_NULL_HANDLE ||
      image_view == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanImage requires successful initialization");
  }

  auto command_buffer_result =
      VulkanCommandBuffer::BeginOneTimeCommands(context_);
  if (!command_buffer_result.ok()) {
    return command_buffer_result.status();
  }
  auto command_buffer_owner = std::move(*command_buffer_result);
  auto& command_buffer = *command_buffer_owner;

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags source_stage;
  VkPipelineStageFlags destination_stage;

  if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    return Status(ErrorCode::kInvalidArgument,
                  "unsupported layout transition!");
  }

  vkCmdPipelineBarrier(command_buffer.buffer(), source_stage, destination_stage, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  {
    const core::Status status = command_buffer.EndOneTimeCommands();
    if (!status.ok()) {
      return status;
    }
  }

  return Status();
}

Status VulkanImage::GenerateMipmaps() {
  if (context_ == nullptr || image == VK_NULL_HANDLE ||
      image_view == VK_NULL_HANDLE) {
    return Status(ErrorCode::kInvalidState,
                  "VulkanImage requires successful initialization");
  }

  auto command_buffer_result =
      VulkanCommandBuffer::BeginOneTimeCommands(context_);
  if (!command_buffer_result.ok()) {
    return command_buffer_result.status();
  }
  auto command_buffer_owner = std::move(*command_buffer_result);
  auto& command_buffer = *command_buffer_owner;

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mip_width = static_cast<int32_t>(image_width);
  int32_t mip_height = static_cast<int32_t>(image_height);

  for (uint32_t i = 1; i < mip_levels_; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer.buffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkImageBlit blit{};
    // src
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {mip_width, mip_height, 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    // dst
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1,
                          1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(command_buffer.buffer(), image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer.buffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &barrier);

    if (mip_width > 1) {
      mip_width /= 2;
    }
    if (mip_height > 1) {
      mip_height /= 2;
    }
  }

  barrier.subresourceRange.baseMipLevel = mip_levels_ - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(command_buffer.buffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);

  {
    const core::Status status = command_buffer.EndOneTimeCommands();
    if (!status.ok()) {
      return status;
    }
  }

  return Status();
}

}  // namespace vulkan
}  // namespace core
