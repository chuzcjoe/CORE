#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

#include "VulkanBuffer.h"
#include "VulkanBufferTransfer.h"
#include "VulkanContext.h"

namespace core {
namespace test {

class VulkanBufferTransferTest : public testing::Test {
 protected:
  void SetUp() override { context_.Init(); }

  std::unique_ptr<vulkan::VulkanBuffer> MakeBuffer(
      VkDeviceSize size = 64,
      VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT) {
    return std::make_unique<vulkan::VulkanBuffer>(
        &context_, size, usage,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  }

  vulkan::VulkanContext context_{true, vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE};
};

TEST_F(VulkanBufferTransferTest, UploadsAndReadsBackAnOrderedChain) {
  std::unique_ptr<vulkan::VulkanBuffer> source = MakeBuffer();
  std::unique_ptr<vulkan::VulkanBuffer> result = MakeBuffer();
  vulkan::VulkanBuffer allocation(
      &context_, 64, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vulkan::VulkanBuffer device;
  device = std::move(allocation);
  source->MapData([](void* data) {
    uint32_t* words = static_cast<uint32_t*>(data);
    for (uint32_t index = 0; index < 16; ++index) {
      words[index] = index * 7 + 3;
    }
  });
  std::array<vulkan::BatchBufferCopy, 2> copies{};
  copies[0].source = source.get();
  copies[0].destination = &device;
  copies[1].source = &device;
  copies[1].destination = result.get();

  vulkan::VulkanBuffer::CopyBuffers(copies);

  result->MapData([](void* data) {
    const uint32_t* words = static_cast<const uint32_t*>(data);
    for (uint32_t index = 0; index < 16; ++index) {
      EXPECT_EQ(words[index], index * 7 + 3);
    }
  });
}

TEST_F(VulkanBufferTransferTest, CopiesSourceRemainderWithOffsetsAndPreservesOtherBytes) {
  std::unique_ptr<vulkan::VulkanBuffer> source = MakeBuffer(16);
  std::unique_ptr<vulkan::VulkanBuffer> destination = MakeBuffer(32);
  source->MapData([](void* data) {
    uint32_t* words = static_cast<uint32_t*>(data);
    for (uint32_t index = 0; index < 4; ++index) {
      words[index] = index + 10;
    }
  });
  destination->MapData([](void* data) {
    uint32_t* words = static_cast<uint32_t*>(data);
    for (uint32_t index = 0; index < 8; ++index) {
      words[index] = 99;
    }
  });
  const vulkan::BatchBufferCopy copy{
      .source = source.get(), .destination = destination.get(),
      .source_offset = 4, .destination_offset = 8, .size = VK_WHOLE_SIZE};
  vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1));
  destination->MapData([](void* data) {
    const uint32_t* words = static_cast<uint32_t*>(data);
    const std::array<uint32_t, 8> expected = {99, 99, 11, 12, 13, 99, 99, 99};
    for (size_t index = 0; index < expected.size(); ++index) {
      EXPECT_EQ(words[index], expected[index]);
    }
  });
}

TEST_F(VulkanBufferTransferTest, AllowsNonOverlappingSelfCopyAndRejectsOverlap) {
  std::unique_ptr<vulkan::VulkanBuffer> buffer = MakeBuffer();
  buffer->MapData([](void* data) { *static_cast<uint32_t*>(data) = 123; });
  vulkan::BatchBufferCopy copy{
      .source = buffer.get(), .destination = buffer.get(),
      .source_offset = 0, .destination_offset = 4, .size = 4};
  vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1));
  buffer->MapData([](void* data) { EXPECT_EQ(static_cast<uint32_t*>(data)[1], 123U); });
  copy.size = 8;
  EXPECT_THROW(vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1)), std::invalid_argument);
}

TEST_F(VulkanBufferTransferTest, RejectsInvalidRangesBeforeExecutingAnyCopy) {
  std::unique_ptr<vulkan::VulkanBuffer> source = MakeBuffer();
  std::unique_ptr<vulkan::VulkanBuffer> destination = MakeBuffer();
  source->MapData([](void* data) { *static_cast<uint32_t*>(data) = 42; });
  destination->MapData([](void* data) { *static_cast<uint32_t*>(data) = 0; });
  std::array<vulkan::BatchBufferCopy, 2> copies{};
  copies[0].source = source.get();
  copies[0].destination = destination.get();
  copies[1] = copies[0];
  for (const VkDeviceSize size : {VkDeviceSize{0}, VkDeviceSize{65},
                                 std::numeric_limits<VkDeviceSize>::max() - 1}) {
    copies[1].size = size;
    EXPECT_THROW(vulkan::VulkanBuffer::CopyBuffers(copies), std::out_of_range);
  }
  copies[1].size = 4;
  copies[1].source_offset = 64;
  EXPECT_THROW(vulkan::VulkanBuffer::CopyBuffers(copies), std::out_of_range);
  copies[1].source_offset = 0;
  copies[1].destination_offset = 63;
  EXPECT_THROW(vulkan::VulkanBuffer::CopyBuffers(copies), std::out_of_range);
  destination->MapData([](void* data) { EXPECT_EQ(*static_cast<uint32_t*>(data), 0U); });
}

TEST_F(VulkanBufferTransferTest, RejectsMissingBuffersUsageAndDifferentContexts) {
  std::unique_ptr<vulkan::VulkanBuffer> source = MakeBuffer();
  std::unique_ptr<vulkan::VulkanBuffer> wrong_usage =
      MakeBuffer(64, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
  vulkan::BatchBufferCopy copy{};
  EXPECT_THROW(vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1)), std::invalid_argument);
  copy.source = source.get();
  copy.destination = wrong_usage.get();
  EXPECT_THROW(vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1)), std::invalid_argument);
  copy.source = wrong_usage.get();
  copy.destination = source.get();
  EXPECT_THROW(vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1)), std::invalid_argument);
  vulkan::VulkanContext other_context(true, vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
  other_context.Init();
  vulkan::VulkanBuffer other(&other_context, 64, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  copy.source = source.get();
  copy.destination = &other;
  EXPECT_THROW(vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1)), std::invalid_argument);
}

TEST_F(VulkanBufferTransferTest, EmptyBatchIsANoOpAndLegacyCopyStillWorks) {
  EXPECT_NO_THROW(vulkan::VulkanBuffer::CopyBuffers({}));
  std::unique_ptr<vulkan::VulkanBuffer> source = MakeBuffer();
  std::unique_ptr<vulkan::VulkanBuffer> destination = MakeBuffer();
  source->MapData([](void* data) { *static_cast<uint32_t*>(data) = 321; });
  source->CopyToBuffer(*destination);
  destination->MapData([](void* data) { EXPECT_EQ(*static_cast<uint32_t*>(data), 321U); });
}

TEST_F(VulkanBufferTransferTest, ReusesSessionAcrossChangingBatchesAndBufferLifetimes) {
  vulkan::VulkanBufferTransfer transfer(context_);
  for (uint32_t frame = 0; frame < 24; ++frame) {
    // Destroy each batch's resources before the next call. Reset must handle a
    // command buffer invalidated by destruction of its previous references.
    std::unique_ptr<vulkan::VulkanBuffer> source = MakeBuffer();
    std::unique_ptr<vulkan::VulkanBuffer> destination = MakeBuffer();
    source->MapData([frame](void* data) {
      uint32_t* words = static_cast<uint32_t*>(data);
      for (uint32_t index = 0; index < 16; ++index) {
        words[index] = frame * 100 + index;
      }
    });
    destination->MapData([](void* data) {
      std::fill_n(static_cast<uint32_t*>(data), 16, 9999U);
    });
    std::array<vulkan::BatchBufferCopy, 3> copies{};
    const size_t count = 1 + frame % copies.size();
    for (size_t index = 0; index < count; ++index) {
      copies[index] = vulkan::BatchBufferCopy{
          .source = source.get(), .destination = destination.get(),
          .source_offset = index * 4, .destination_offset = 8, .size = 4};
    }
    transfer.CopyBuffers(std::span(copies.data(), count));
    destination->MapData([frame, count](void* data) {
      const uint32_t* words = static_cast<const uint32_t*>(data);
      for (uint32_t index = 0; index < 16; ++index) {
        const uint32_t expected = index == 2 ? frame * 100 + static_cast<uint32_t>(count - 1)
                                             : 9999U;
        EXPECT_EQ(words[index], expected);
      }
    });
    transfer.CopyBuffers({});
  }
}

TEST_F(VulkanBufferTransferTest, RejectedBatchPreservesDataAndSessionRemainsUsable) {
  vulkan::VulkanBufferTransfer transfer(context_);
  std::unique_ptr<vulkan::VulkanBuffer> source = MakeBuffer();
  std::unique_ptr<vulkan::VulkanBuffer> destination = MakeBuffer();
  std::unique_ptr<vulkan::VulkanBuffer> readback = MakeBuffer();
  source->MapData([](void* data) { *static_cast<uint32_t*>(data) = 42; });
  destination->MapData([](void* data) { *static_cast<uint32_t*>(data) = 17; });
  std::array<vulkan::BatchBufferCopy, 2> copies{};
  copies[0].source = source.get();
  copies[0].destination = destination.get();
  copies[1].source = destination.get();
  copies[1].destination = readback.get();
  copies[1].size = 65;
  EXPECT_THROW(transfer.CopyBuffers(copies), std::out_of_range);
  destination->MapData([](void* data) { EXPECT_EQ(*static_cast<uint32_t*>(data), 17U); });
  copies[1].size = VK_WHOLE_SIZE;
  transfer.CopyBuffers(copies);
  readback->MapData([](void* data) { EXPECT_EQ(*static_cast<uint32_t*>(data), 42U); });
  copies[1].destination = nullptr;
  EXPECT_THROW(transfer.CopyBuffers(copies), std::invalid_argument);
  copies[1].destination = readback.get();
  source->MapData([](void* data) { *static_cast<uint32_t*>(data) = 81; });
  transfer.CopyBuffers(copies);
  readback->MapData([](void* data) { EXPECT_EQ(*static_cast<uint32_t*>(data), 81U); });
}

TEST_F(VulkanBufferTransferTest, SessionRejectsBatchFromAnotherContext) {
  vulkan::VulkanContext other(true, vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
  other.Init();
  vulkan::VulkanBufferTransfer transfer(other);
  std::unique_ptr<vulkan::VulkanBuffer> source = MakeBuffer();
  std::unique_ptr<vulkan::VulkanBuffer> destination = MakeBuffer();
  const vulkan::BatchBufferCopy copy{
      .source = source.get(), .destination = destination.get(),
      .source_offset = 0, .destination_offset = 0, .size = VK_WHOLE_SIZE};
  EXPECT_THROW(transfer.CopyBuffers(std::span(&copy, 1)), std::invalid_argument);
  EXPECT_NO_THROW(transfer.CopyBuffers({}));
}

}  // namespace test
}  // namespace core
