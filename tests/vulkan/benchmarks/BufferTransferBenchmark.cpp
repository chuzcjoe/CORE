#include <algorithm>
#include <chrono>
#include <cstdio>
#include <exception>
#include <memory>
#include <stdexcept>
#include <vector>

#include "VulkanBuffer.h"
#include "VulkanContext.h"

namespace {

using core::vulkan::BatchBufferCopy;
using core::vulkan::VulkanBuffer;
using core::vulkan::VulkanContext;

void Benchmark(VulkanContext& context, size_t count, VkDeviceSize bytes) {
  std::vector<std::unique_ptr<VulkanBuffer>> sources;
  std::vector<std::unique_ptr<VulkanBuffer>> destinations;
  std::vector<BatchBufferCopy> copies(count);
  for (size_t index = 0; index < count; ++index) {
    sources.push_back(std::make_unique<VulkanBuffer>(
        &context, bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
    destinations.push_back(std::make_unique<VulkanBuffer>(
        &context, bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
    sources.back()->MapData([index, bytes](void* data) {
      uint32_t* words = static_cast<uint32_t*>(data);
      for (VkDeviceSize word = 0; word < bytes / sizeof(uint32_t); ++word) {
        words[word] = static_cast<uint32_t>(word + index * 17);
      }
    });
    copies[index].source = sources.back().get();
    copies[index].destination = destinations.back().get();
  }
  VulkanBuffer readback(&context, bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  const auto verify = [&]() {
    for (size_t index = 0; index < count; ++index) {
      destinations[index]->CopyToBuffer(readback);
      readback.MapData([index, bytes](void* data) {
        const uint32_t* words = static_cast<const uint32_t*>(data);
        for (VkDeviceSize word = 0; word < bytes / sizeof(uint32_t); ++word) {
          if (words[word] != static_cast<uint32_t>(word + index * 17)) {
            throw std::runtime_error("Transfer benchmark data mismatch");
          }
        }
      });
    }
  };
  const auto run = [&](bool batch) {
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    if (batch) {
      VulkanBuffer::CopyBuffers(copies);
    } else {
      for (size_t index = 0; index < count; ++index) {
        sources[index]->CopyToBuffer(*destinations[index]);
      }
    }
    return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start)
        .count();
  };

  constexpr int kWarmups = 5;
  constexpr int kSamples = 31;
  std::vector<double> legacy_samples;
  std::vector<double> batch_samples;
  for (int sample = -kWarmups; sample < kSamples; ++sample) {
    // Alternate order to reduce systematic temperature/cache/order bias.
    for (int turn = 0; turn < 2; ++turn) {
      const bool batch = ((sample + kWarmups + turn) % 2) == 0;
      const double elapsed = run(batch);
      if (sample >= 0) {
        (batch ? batch_samples : legacy_samples).push_back(elapsed);
      }
      if (sample == -kWarmups || sample == kSamples - 1) {
        verify();  // Full data check for both paths, outside the timed interval.
      }
    }
  }
  std::sort(legacy_samples.begin(), legacy_samples.end());
  std::sort(batch_samples.begin(), batch_samples.end());
  std::printf("copies=%zu bytes_each=%llu warmups=%d samples=%d "
              "legacy_us[p10,median,p90]=[%.2f,%.2f,%.2f] "
              "batch_us[p10,median,p90]=[%.2f,%.2f,%.2f] speedup=%.2fx verified=yes\n",
              count, static_cast<unsigned long long>(bytes), kWarmups, kSamples,
              legacy_samples[3], legacy_samples[15], legacy_samples[27], batch_samples[3],
              batch_samples[15], batch_samples[27], legacy_samples[15] / batch_samples[15]);
}

}  // namespace

int main() {
  try {
    VulkanContext context(true, core::vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
    context.Init();
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(context.physical_device, &properties);
    std::printf("GPU: %s; wall time includes recording, pool/fence allocation, submit and wait; "
                "validation requested; buffer allocation/fill/readback excluded\n",
                properties.deviceName);
    Benchmark(context, 3, 4096);
    Benchmark(context, 16, 65536);
    Benchmark(context, 16, 1048576);
  } catch (const std::exception& error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
  return 0;
}
