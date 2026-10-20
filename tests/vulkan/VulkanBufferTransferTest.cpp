#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

#include "Status.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"

namespace core {
namespace test {

class VulkanBufferTransferTest : public testing::Test {
 protected:
  void SetUp() override {
    auto result =
        vulkan::VulkanContext::Create(true, vulkan::QueueFamilyType::Compute);
    ASSERT_TRUE(result.ok()) << result.status().message();
    context_ = std::move(*result);
    ASSERT_TRUE(context_->Init().ok());
  }

  Result<std::unique_ptr<vulkan::VulkanBuffer>> MakeBuffer(
      VkDeviceSize size = 64,
      VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT) {
    auto created_resource_0 =
        vulkan::VulkanBuffer::Create(context_.get(), size, usage,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!created_resource_0.ok()) {
      return Result<std::unique_ptr<vulkan::VulkanBuffer>>(
          created_resource_0.status());
    }
    return Result<std::unique_ptr<vulkan::VulkanBuffer>>(
        std::move(*created_resource_0));
  }

  std::unique_ptr<vulkan::VulkanContext> context_;
};

TEST_F(VulkanBufferTransferTest, UploadsAndReadsBackAnOrderedChain) {
  auto source_result = MakeBuffer();
  ASSERT_TRUE(source_result.ok()) << source_result.status().message();
  auto source = std::move(*source_result);
  auto result_result = MakeBuffer();
  ASSERT_TRUE(result_result.ok()) << result_result.status().message();
  auto result = std::move(*result_result);

  auto allocation_result = vulkan::VulkanBuffer::Create(
      context_.get(), 64,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (!allocation_result.ok()) {
    FAIL() << allocation_result.status().message();
  }
  std::unique_ptr<vulkan::VulkanBuffer> allocation_owner =
      std::move(*allocation_result);
  vulkan::VulkanBuffer& allocation = *allocation_owner;
  vulkan::VulkanBuffer device;
  device = std::move(allocation);
  {
    const core::Status status = source->MapData([](void* data) {
      uint32_t* words = static_cast<uint32_t*>(data);
      for (uint32_t index = 0; index < 16; ++index) {
        words[index] = index * 7 + 3;
      }
    });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  std::array<vulkan::BatchBufferCopy, 2> copies{};
  copies[0].source = source.get();
  copies[0].destination = &device;
  copies[1].source = &device;
  copies[1].destination = result.get();

  {
    const core::Status status = vulkan::VulkanBuffer::CopyBuffers(copies);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  {
    const core::Status status = result->MapData([](void* data) {
      const uint32_t* words = static_cast<const uint32_t*>(data);
      for (uint32_t index = 0; index < 16; ++index) {
        EXPECT_EQ(words[index], index * 7 + 3);
      }
    });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
}

TEST_F(VulkanBufferTransferTest, CopiesSourceRemainderWithOffsetsAndPreservesOtherBytes) {
  auto source_result = MakeBuffer(16);
  ASSERT_TRUE(source_result.ok()) << source_result.status().message();
  auto source = std::move(*source_result);
  auto destination_result = MakeBuffer(32);
  ASSERT_TRUE(destination_result.ok()) << destination_result.status().message();
  auto destination = std::move(*destination_result);
  {
    const core::Status status = source->MapData([](void* data) {
      uint32_t* words = static_cast<uint32_t*>(data);
      for (uint32_t index = 0; index < 4; ++index) {
        words[index] = index + 10;
      }
    });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = destination->MapData([](void* data) {
      uint32_t* words = static_cast<uint32_t*>(data);
      for (uint32_t index = 0; index < 8; ++index) {
        words[index] = 99;
      }
    });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  const vulkan::BatchBufferCopy copy{
      .source = source.get(), .destination = destination.get(),
      .source_offset = 4, .destination_offset = 8, .size = VK_WHOLE_SIZE};
  {
    const core::Status status =
        vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1));
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = destination->MapData([](void* data) {
      const uint32_t* words = static_cast<uint32_t*>(data);
      const std::array<uint32_t, 8> expected = {99, 99, 11, 12, 13, 99, 99, 99};
      for (size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(words[index], expected[index]);
      }
    });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
}

TEST_F(VulkanBufferTransferTest, AllowsNonOverlappingSelfCopyAndRejectsOverlap) {
  auto buffer_result = MakeBuffer();
  ASSERT_TRUE(buffer_result.ok()) << buffer_result.status().message();
  auto buffer = std::move(*buffer_result);
  {
    const core::Status status = buffer->MapData(
        [](void* data) { *static_cast<uint32_t*>(data) = 123; });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  vulkan::BatchBufferCopy copy{
      .source = buffer.get(), .destination = buffer.get(),
      .source_offset = 0, .destination_offset = 4, .size = 4};
  {
    const core::Status status =
        vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1));
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = buffer->MapData(
        [](void* data) { EXPECT_EQ(static_cast<uint32_t*>(data)[1], 123U); });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  copy.size = 8;
  EXPECT_EQ((vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1))).code(),
            core::ErrorCode::kInvalidArgument);
}

TEST_F(VulkanBufferTransferTest, RejectsInvalidRangesBeforeExecutingAnyCopy) {
  auto source_result = MakeBuffer();
  ASSERT_TRUE(source_result.ok()) << source_result.status().message();
  auto source = std::move(*source_result);
  auto destination_result = MakeBuffer();
  ASSERT_TRUE(destination_result.ok()) << destination_result.status().message();
  auto destination = std::move(*destination_result);
  {
    const core::Status status =
        source->MapData([](void* data) { *static_cast<uint32_t*>(data) = 42; });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = destination->MapData(
        [](void* data) { *static_cast<uint32_t*>(data) = 0; });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  std::array<vulkan::BatchBufferCopy, 2> copies{};
  copies[0].source = source.get();
  copies[0].destination = destination.get();
  copies[1] = copies[0];
  for (const VkDeviceSize size : {VkDeviceSize{0}, VkDeviceSize{65},
                                 std::numeric_limits<VkDeviceSize>::max() - 1}) {
    copies[1].size = size;
    EXPECT_EQ((vulkan::VulkanBuffer::CopyBuffers(copies)).code(),
              core::ErrorCode::kOutOfRange);
  }
  copies[1].size = 4;
  copies[1].source_offset = 64;
  EXPECT_EQ((vulkan::VulkanBuffer::CopyBuffers(copies)).code(),
            core::ErrorCode::kOutOfRange);
  copies[1].source_offset = 0;
  copies[1].destination_offset = 63;
  EXPECT_EQ((vulkan::VulkanBuffer::CopyBuffers(copies)).code(),
            core::ErrorCode::kOutOfRange);
  {
    const core::Status status = destination->MapData(
        [](void* data) { EXPECT_EQ(*static_cast<uint32_t*>(data), 0U); });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
}

TEST_F(VulkanBufferTransferTest, RejectsMissingBuffersUsageAndDifferentContexts) {
  auto source_result = MakeBuffer();
  ASSERT_TRUE(source_result.ok()) << source_result.status().message();
  auto source = std::move(*source_result);
  auto wrong_usage_result = MakeBuffer(64, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
  ASSERT_TRUE(wrong_usage_result.ok()) << wrong_usage_result.status().message();
  auto wrong_usage = std::move(*wrong_usage_result);
  vulkan::BatchBufferCopy copy{};
  EXPECT_EQ((vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1))).code(),
            core::ErrorCode::kInvalidArgument);
  copy.source = source.get();
  copy.destination = wrong_usage.get();
  EXPECT_EQ((vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1))).code(),
            core::ErrorCode::kInvalidArgument);
  copy.source = wrong_usage.get();
  copy.destination = source.get();
  EXPECT_EQ((vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1))).code(),
            core::ErrorCode::kInvalidArgument);

  auto other_context_result = vulkan::VulkanContext::Create(
      true, vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
  if (!other_context_result.ok()) {
    FAIL() << other_context_result.status().message();
  }
  std::unique_ptr<vulkan::VulkanContext> other_context_owner =
      std::move(*other_context_result);
  vulkan::VulkanContext& other_context = *other_context_owner;
  {
    const core::Status status = other_context.Init();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  auto other_result = vulkan::VulkanBuffer::Create(
      &other_context, 64, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (!other_result.ok()) {
    FAIL() << other_result.status().message();
  }
  std::unique_ptr<vulkan::VulkanBuffer> other_owner = std::move(*other_result);
  vulkan::VulkanBuffer& other = *other_owner;
  copy.source = source.get();
  copy.destination = &other;
  EXPECT_EQ((vulkan::VulkanBuffer::CopyBuffers(std::span(&copy, 1))).code(),
            core::ErrorCode::kInvalidArgument);
}

TEST_F(VulkanBufferTransferTest, EmptyBatchIsANoOpAndLegacyCopyStillWorks) {
  EXPECT_TRUE((vulkan::VulkanBuffer::CopyBuffers({})).ok());
  auto source_result = MakeBuffer();
  ASSERT_TRUE(source_result.ok()) << source_result.status().message();
  auto source = std::move(*source_result);
  auto destination_result = MakeBuffer();
  ASSERT_TRUE(destination_result.ok()) << destination_result.status().message();
  auto destination = std::move(*destination_result);
  {
    const core::Status status = source->MapData(
        [](void* data) { *static_cast<uint32_t*>(data) = 321; });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = source->CopyToBuffer(*destination);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = destination->MapData(
        [](void* data) { EXPECT_EQ(*static_cast<uint32_t*>(data), 321U); });
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
}

}  // namespace test
}  // namespace core
