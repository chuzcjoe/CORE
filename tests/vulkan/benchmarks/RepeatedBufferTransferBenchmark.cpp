#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <exception>
#include <memory>
#include <stdexcept>
#include <vector>

#include "VulkanBuffer.h"
#include "VulkanBufferTransfer.h"
#include "VulkanContext.h"

namespace {

using core::vulkan::BatchBufferCopy;
using core::vulkan::VulkanBuffer;
using core::vulkan::VulkanBufferTransfer;
using core::vulkan::VulkanContext;

double ThreadCpuMicroseconds() {
  timespec time{};
  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time) != 0) {
    throw std::runtime_error("Cannot read calling-thread CPU clock");
  }
  return static_cast<double>(time.tv_sec) * 1000000.0 +
         static_cast<double>(time.tv_nsec) / 1000.0;
}

void Benchmark(VulkanContext& context, size_t count, VkDeviceSize bytes) {
  constexpr VkMemoryPropertyFlags kHostMemory =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  std::vector<std::unique_ptr<VulkanBuffer>> sources;
  std::vector<std::unique_ptr<VulkanBuffer>> destinations;
  std::vector<BatchBufferCopy> copies(count);
  for (size_t index = 0; index < count; ++index) {
    sources.push_back(std::make_unique<VulkanBuffer>(
        &context, bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, kHostMemory));
    destinations.push_back(std::make_unique<VulkanBuffer>(
        &context, bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
    copies[index].source = sources.back().get();
    copies[index].destination = destinations.back().get();
  }
  VulkanBuffer readback(&context, bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, kHostMemory);
  VulkanBufferTransfer transfer(context);
  VulkanBufferTransfer readback_transfer(context);
  const auto fill = [&](uint32_t generation) {
    for (size_t index = 0; index < count; ++index) {
      sources[index]->MapData([index, bytes, generation](void* data) {
        uint32_t* words = static_cast<uint32_t*>(data);
        for (VkDeviceSize word = 0; word < bytes / sizeof(uint32_t); ++word) {
          words[word] = generation + static_cast<uint32_t>(word + index * 17);
        }
      });
    }
  };
  const auto verify = [&](uint32_t generation) {
    for (size_t index = 0; index < count; ++index) {
      const BatchBufferCopy copy{
          .source = destinations[index].get(), .destination = &readback,
          .source_offset = 0, .destination_offset = 0, .size = VK_WHOLE_SIZE};
      readback_transfer.CopyBuffers(std::span(&copy, 1));
      bool matches = true;
      readback.MapData([index, bytes, generation, &matches](void* data) {
        const uint32_t* words = static_cast<const uint32_t*>(data);
        for (VkDeviceSize word = 0; word < bytes / sizeof(uint32_t); ++word) {
          if (words[word] != generation + static_cast<uint32_t>(word + index * 17)) {
            matches = false;
          }
        }
      });
      if (!matches) {
        throw std::runtime_error("Repeated transfer benchmark data mismatch");
      }
    }
  };

  constexpr int kWarmups = 5;
  constexpr int kSamples = 31;
  std::array<std::vector<double>, 2> samples;
  std::array<std::vector<double>, 2> cpu_samples;
  uint32_t generation = 0;
  for (int sample = -kWarmups; sample < kSamples; ++sample) {
    for (int turn = 0; turn < 2; ++turn) {
      const size_t path = static_cast<size_t>((sample + kWarmups + turn) % 2);
      ++generation;
      fill(generation);  // Each call must transfer fresh data.
      const double cpu_start = ThreadCpuMicroseconds();
      const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
      if (path == 0) {
        VulkanBuffer::CopyBuffers(copies);
      } else {
        transfer.CopyBuffers(copies);
      }
      const double elapsed =
          std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start)
              .count();
      const double cpu_elapsed = ThreadCpuMicroseconds() - cpu_start;
      if (sample >= 0) {
        samples[path].push_back(elapsed);
        cpu_samples[path].push_back(cpu_elapsed);
      }
      verify(generation);  // Check every output word after every timed call.
    }
  }
  for (std::vector<double>& values : samples) {
    std::sort(values.begin(), values.end());
  }
  for (std::vector<double>& values : cpu_samples) {
    std::sort(values.begin(), values.end());
  }
  std::printf("copies=%zu bytes_each=%llu warmups=%d samples=%d "
              "one_shot_us[p10,median,p90]=[%.2f,%.2f,%.2f] "
              "reused_us[p10,median,p90]=[%.2f,%.2f,%.2f] ratio=%.2fx verified=yes\n",
              count, static_cast<unsigned long long>(bytes), kWarmups, kSamples,
              samples[0][3], samples[0][15], samples[0][27], samples[1][3], samples[1][15],
              samples[1][27], samples[0][15] / samples[1][15]);
  std::printf("calling_thread_cpu_us[p10,median,p90] one_shot=[%.2f,%.2f,%.2f] "
              "reused=[%.2f,%.2f,%.2f] ratio=%.2fx\n",
              cpu_samples[0][3], cpu_samples[0][15], cpu_samples[0][27], cpu_samples[1][3],
              cpu_samples[1][15], cpu_samples[1][27], cpu_samples[0][15] / cpu_samples[1][15]);
}

}  // namespace

int main() {
  try {
    VulkanContext context(true, core::vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
    context.Init();
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(context.physical_device, &properties);
    std::printf("GPU: %s; validation requested; wall time includes validation, recording, "
                "submission and wait; one-shot also includes session creation/destruction; "
                "reused session setup and all fill/readback excluded\n", properties.deviceName);
    Benchmark(context, 1, 4096);
    Benchmark(context, 3, 4096);
    Benchmark(context, 16, 65536);
    Benchmark(context, 16, 1048576);
  } catch (const std::exception& error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
  return 0;
}
