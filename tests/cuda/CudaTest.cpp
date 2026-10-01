#include <cuda_runtime.h>
#include <gtest/gtest.h>

#include <vector>

namespace core {
namespace test {
namespace {

__global__ void VecAddKernel(const float* lhs, const float* rhs, float* output,
                             const size_t element_count) {
  const size_t index = static_cast<size_t>(blockIdx.x) * static_cast<size_t>(blockDim.x) +
                       static_cast<size_t>(threadIdx.x);
  if (index < element_count) {
    output[index] = lhs[index] + rhs[index];
  }
}

}  // namespace

TEST(Cuda, VecAdd) {
  constexpr size_t kElementCount = 1024;
  constexpr size_t kBufferSize = kElementCount * sizeof(float);
  constexpr int kThreadsPerBlock = 256;
  constexpr int kBlockCount =
      static_cast<int>((kElementCount + kThreadsPerBlock - 1) / kThreadsPerBlock);

  std::vector<float> lhs(kElementCount, 1.5f);
  std::vector<float> rhs(kElementCount, 2.5f);
  std::vector<float> output(kElementCount, 0.0f);

  float* device_lhs = nullptr;
  float* device_rhs = nullptr;
  float* device_output = nullptr;

  ASSERT_EQ(cudaMalloc(&device_lhs, kBufferSize), cudaSuccess);
  ASSERT_EQ(cudaMalloc(&device_rhs, kBufferSize), cudaSuccess);
  ASSERT_EQ(cudaMalloc(&device_output, kBufferSize), cudaSuccess);

  ASSERT_EQ(cudaMemcpy(device_lhs, lhs.data(), kBufferSize, cudaMemcpyHostToDevice), cudaSuccess);
  ASSERT_EQ(cudaMemcpy(device_rhs, rhs.data(), kBufferSize, cudaMemcpyHostToDevice), cudaSuccess);

  VecAddKernel<<<kBlockCount, kThreadsPerBlock>>>(device_lhs, device_rhs, device_output,
                                                  kElementCount);
  ASSERT_EQ(cudaGetLastError(), cudaSuccess);
  ASSERT_EQ(cudaDeviceSynchronize(), cudaSuccess);

  ASSERT_EQ(cudaMemcpy(output.data(), device_output, kBufferSize, cudaMemcpyDeviceToHost),
            cudaSuccess);

  for (const float value : output) {
    EXPECT_FLOAT_EQ(value, 4.0f);
  }

  EXPECT_EQ(cudaFree(device_output), cudaSuccess);
  EXPECT_EQ(cudaFree(device_rhs), cudaSuccess);
  EXPECT_EQ(cudaFree(device_lhs), cudaSuccess);
}

}  // namespace test
}  // namespace core
