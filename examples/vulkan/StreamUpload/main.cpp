#include <array>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <stdexcept>

#include "VulkanBuffer.h"
#include "VulkanBufferTransfer.h"
#include "VulkanContext.h"

int main() {
  try {
    using core::vulkan::VulkanBuffer;
    core::vulkan::VulkanContext context(true, core::vulkan::QueueFamilyType::Compute,
                                       VK_NULL_HANDLE);
    context.Init();
    constexpr uint32_t kWords = 4096;
    constexpr uint32_t kFrames = 60;
    constexpr VkDeviceSize kBytes = kWords * sizeof(uint32_t);
    constexpr VkMemoryPropertyFlags kHostMemory =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanBuffer staging(&context, kBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, kHostMemory);
    VulkanBuffer device(&context, kBytes,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VulkanBuffer readback(&context, kBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, kHostMemory);
    core::vulkan::VulkanBufferTransfer transfer(context);
    std::array<core::vulkan::BatchBufferCopy, 2> copies{};
    copies[0].source = &staging;
    copies[0].destination = &device;
    copies[1].source = &device;
    copies[1].destination = &readback;

    for (uint32_t frame = 0; frame < kFrames; ++frame) {
      staging.MapData([frame](void* data) {
        uint32_t* words = static_cast<uint32_t*>(data);
        for (uint32_t index = 0; index < kWords; ++index) {
          words[index] = frame * kWords + index;
        }
      });
      transfer.CopyBuffers(copies);
      bool matches = true;
      readback.MapData([frame, &matches](void* data) {
        const uint32_t* words = static_cast<const uint32_t*>(data);
        for (uint32_t index = 0; index < kWords; ++index) {
          if (words[index] != frame * kWords + index) {
            matches = false;
          }
        }
      });
      if (!matches) {
        throw std::runtime_error("Streaming upload readback mismatch");
      }
    }
    std::printf("Verified %u uploads of %u words using one transfer session\n", kFrames, kWords);
  } catch (const std::exception& error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
  return 0;
}
