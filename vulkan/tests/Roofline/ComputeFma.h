#pragma once

#include "VulkanBuffer.h"
#include "VulkanCompute.h"

namespace core {
namespace vulkan {

// Pure-ALU microbenchmark: every thread runs long register-resident FMA
// chains with no memory traffic inside the loop. Used to measure the GPU's
// peak compute throughput for the roofline model.
class ComputeFma : public VulkanCompute {
 public:
  // Must match the shader's local_size_x.
  static constexpr uint32_t kLocalSizeX = 256;
  // 32 fused multiply-adds (64 FLOP) per loop iteration per thread; must
  // match the unrolled loop body in ComputeFma.comp.
  static constexpr uint64_t kFlopPerIteration = 64;

  ComputeFma(VulkanContext* context, VulkanBuffer& dst, const int iterations,
             const uint32_t group_count);

  void Init() override;
  void Run(const VkCommandBuffer command_buffer);

 protected:
  std::vector<BindingInfo> GetBindingInfo() const override;
  const std::vector<uint32_t>& LoadShaderCode() const override;

 private:
  VulkanBuffer& dst_buffer_;
  VulkanBuffer uniform_buffer_;
  struct UniformData {
    int iterations;
  } uniform_data_;
  uint32_t group_count_;
};

}  // namespace vulkan
}  // namespace core
