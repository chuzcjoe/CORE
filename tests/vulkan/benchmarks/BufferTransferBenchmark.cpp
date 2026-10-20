#include <algorithm>
#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>

#include "Status.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"

namespace {

using core::vulkan::BatchBufferCopy;
using core::vulkan::VulkanBuffer;
using core::vulkan::VulkanContext;

core::Status Benchmark(VulkanContext& context, size_t count,
                       VkDeviceSize bytes) {
  std::vector<std::unique_ptr<VulkanBuffer>> sources;
  std::vector<std::unique_ptr<VulkanBuffer>> destinations;
  std::vector<BatchBufferCopy> copies(count);
  for (size_t index = 0; index < count; ++index) {
    auto created_resource_0 =
        VulkanBuffer::Create(&context, bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!created_resource_0.ok()) {
      return created_resource_0.status();
    }
    sources.push_back(std::move(*created_resource_0));
    auto created_resource_4 = VulkanBuffer::Create(
        &context, bytes,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!created_resource_4.ok()) {
      return created_resource_4.status();
    }
    destinations.push_back(std::move(*created_resource_4));
    const core::Status fill_status =
        sources.back()->MapData([index, bytes](void* data) {
          uint32_t* words = static_cast<uint32_t*>(data);
          for (VkDeviceSize word = 0; word < bytes / sizeof(uint32_t); ++word) {
            words[word] = static_cast<uint32_t>(word + index * 17);
          }
        });
    if (!fill_status.ok()) {
      return fill_status;
    }
    copies[index].source = sources.back().get();
    copies[index].destination = destinations.back().get();
  }

  auto readback_result =
      VulkanBuffer::Create(&context, bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!readback_result.ok()) {
    return readback_result.status();
  }
  std::unique_ptr<VulkanBuffer> readback_owner = std::move(*readback_result);
  VulkanBuffer& readback = *readback_owner;
  const auto verify = [&]() -> core::Status {
    for (size_t index = 0; index < count; ++index) {
      const core::Status copy_status =
          destinations[index]->CopyToBuffer(readback);
      if (!copy_status.ok()) {
        return copy_status;
      }
      bool matches = true;
      const core::Status map_status = readback.MapData([index, bytes,
                                                        &matches](void* data) {
        const uint32_t* words = static_cast<const uint32_t*>(data);
        for (VkDeviceSize word = 0; word < bytes / sizeof(uint32_t); ++word) {
          if (words[word] != static_cast<uint32_t>(word + index * 17)) {
            matches = false;
          }
        }
      });
      if (!map_status.ok()) {
        return map_status;
      }
      if (!matches) {
        return core::Status(core::ErrorCode::kBackend,
                            "Transfer benchmark data mismatch");
      }
    }
    return core::Status();
  };
  const auto run = [&](bool batch) -> core::Result<double> {
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    if (batch) {
      const core::Status status = VulkanBuffer::CopyBuffers(copies);
      if (!status.ok()) {
        return core::Result<double>(status);
      }
    } else {
      for (size_t index = 0; index < count; ++index) {
        const core::Status status =
            sources[index]->CopyToBuffer(*destinations[index]);
        if (!status.ok()) {
          return core::Result<double>(status);
        }
      }
    }
    return core::Result<double>(std::chrono::duration<double, std::micro>(
                                    std::chrono::steady_clock::now() - start)
                                    .count());
  };

  constexpr int kWarmups = 5;
  constexpr int kSamples = 31;
  std::vector<double> legacy_samples;
  std::vector<double> batch_samples;
  for (int sample = -kWarmups; sample < kSamples; ++sample) {
    // Alternate order to reduce systematic temperature/cache/order bias.
    for (int turn = 0; turn < 2; ++turn) {
      const bool batch = ((sample + kWarmups + turn) % 2) == 0;
      const auto elapsed_result = run(batch);
      if (!elapsed_result.ok()) {
        return elapsed_result.status();
      }
      const double elapsed = *elapsed_result;
      if (sample >= 0) {
        (batch ? batch_samples : legacy_samples).push_back(elapsed);
      }
      if (sample == -kWarmups || sample == kSamples - 1) {
        const core::Status status = verify();
        if (!status.ok()) {
          return status;
        }  // Full data check for both paths, outside the timed interval.
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

  return core::Status();
}

}  // namespace

core::Status RunApplication() {
  auto context_result = VulkanContext::Create(
      true, core::vulkan::QueueFamilyType::Compute, VK_NULL_HANDLE);
  if (!context_result.ok()) {
    return context_result.status();
  }
  std::unique_ptr<VulkanContext> context_owner = std::move(*context_result);
  VulkanContext& context = *context_owner;
  {
    const core::Status status = context.Init();
    if (!status.ok()) {
      return status;
    }
  }
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(context.physical_device, &properties);
  std::printf(
      "GPU: %s; wall time includes recording, pool/fence allocation, submit "
      "and wait; "
      "validation requested; buffer allocation/fill/readback excluded\n",
      properties.deviceName);
  {
    const core::Status status = Benchmark(context, 3, 4096);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = Benchmark(context, 16, 65536);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = Benchmark(context, 16, 1048576);
    if (!status.ok()) {
      return status;
    }
  }

  return core::Status();
}

int main() {
  const core::Status status = RunApplication();
  if (!status.ok()) {
    std::fprintf(stderr, "%s\n", status.message().c_str());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
