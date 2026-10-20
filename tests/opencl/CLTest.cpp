#include <gtest/gtest.h>

#include <cmath>
#include <iostream>
#include <random>

#include "CLBuffer.h"
#include "CLCommandQueue.h"
#include "CLContext.h"
#include "CLKernel.h"
#include "CLLoader.h"
#include "CLProgram.h"
#include "Mat.h"
#include "Status.h"

#if defined(__ANDROID__)
#define SHADER_PATH "/data/local/tmp/core/tests/shaders/"
#elif defined(__APPLE__) || defined(__linux__)
#define SHADER_PATH "./tests/shaders/"
#endif

namespace core {
namespace test {

TEST(OpenCL, test1) {
  int init = core::opencl::cl_init();
  if (init) {
    FAIL() << core::Status(core::ErrorCode::kBackend,
                           "Failed to initialize OpenCL loader")
                  .message();
  }

  cl_int error = 0;
  cl_platform_id platform_ids[10];
  cl_uint num_platforms;
  error = clGetPlatformIDs(10, platform_ids, &num_platforms);
  if (error != CL_SUCCESS) {
    FAIL() << core::Status(core::ErrorCode::kBackend, "clGetPlatformIDs failed")
                  .message();
  }
  std::cout << "num platforms: " << num_platforms << std::endl;
}

TEST(OpenCL, VecAdd) {
  // 1. Input data
  const int N = 8;
  std::vector<float> A(N), B(N), C(N);
  for (int i = 0; i < N; ++i) {
    A[i] = static_cast<float>(i);
    B[i] = static_cast<float>(i * 10);
    std::cout << "A[" << i << "] = " << A[i] << ", B[" << i << "] = " << B[i] << '\n';
  }

  // 2. Kernel source embedded as string literal
  const char* kernelSrc = R"CLC(
        __kernel void vec_add(__global const float* A,
                              __global const float* B,
                              __global float* C) {
            int i = get_global_id(0);
            C[i] = A[i] + B[i];
        }
    )CLC";

  // initialize OpenCL
  int init = core::opencl::cl_init();
  if (init) {
    FAIL() << core::Status(core::ErrorCode::kBackend,
                           "Failed to initialize OpenCL loader")
                  .message();
  }

  // 3. Discover platform and device
  cl_int err = CL_SUCCESS;
  cl_uint numPlatforms = 0;
  clGetPlatformIDs(0, nullptr, &numPlatforms);
  std::vector<cl_platform_id> platforms(numPlatforms);
  clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
  cl_platform_id platform = platforms[0];

  cl_uint numDevices = 0;
  clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
  std::vector<cl_device_id> devices(numDevices);
  clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);
  cl_device_id device = devices[0];

  // 4. Create context and command queue
  cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);

  // 5. Build program from embedded source
  size_t length = std::strlen(kernelSrc);
  const char* srcPtr = kernelSrc;
  cl_program program = clCreateProgramWithSource(context, 1, &srcPtr, &length, &err);
  err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);

  if (err != CL_SUCCESS) {
    size_t logSize = 0;
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
    std::vector<char> log(logSize);
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
    FAIL() << core::Status(core::ErrorCode::kBackend,
                           "clBuildProgram failed:\n" + std::string(log.data()))
                  .message();
  }

  cl_kernel kernel = clCreateKernel(program, "vec_add", &err);

  // 6. Create buffers
  cl_mem buffer_a = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   sizeof(float) * N, A.data(), &err);
  cl_mem buffer_b = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   sizeof(float) * N, B.data(), &err);
  cl_mem buffer_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * N, nullptr, &err);

  // 7. Set arguments and launch kernel
  clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer_a);
  clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_b);
  clSetKernelArg(kernel, 2, sizeof(cl_mem), &buffer_c);

  size_t globalWorkSize = N;
  err = clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &globalWorkSize, nullptr, 0, nullptr,
                               nullptr);
  clFinish(queue);

  // 8. Read results
  clEnqueueReadBuffer(queue, buffer_c, CL_TRUE, 0, sizeof(float) * N, C.data(), 0, nullptr,
                      nullptr);

  // 9. Print results
  for (int i = 0; i < N; ++i) {
    std::cout << "C[" << i << "] = " << C[i] << '\n';
  }

  // 10. Cleanup
  clReleaseMemObject(buffer_a);
  clReleaseMemObject(buffer_b);
  clReleaseMemObject(buffer_c);
  clReleaseKernel(kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);
}

TEST(OpenCL, VecAddCLWrapper) {
  const int N = 8;
  std::vector<float> A(N), B(N), C(N);
  for (int i = 0; i < N; ++i) {
    A[i] = static_cast<float>(i);
    B[i] = static_cast<float>(i * 10);
    std::cout << "A[" << i << "] = " << A[i] << ", B[" << i << "] = " << B[i] << '\n';
  }

  // initialize OpenCL
  int init = core::opencl::cl_init();
  if (init) {
    FAIL() << core::Status(core::ErrorCode::kBackend,
                           "Failed to initialize OpenCL loader")
                  .message();
  }

  size_t buffer_size = sizeof(float) * N;

  auto clcontext_result = core::opencl::CLContext::Create();
  if (!clcontext_result.ok()) {
    FAIL() << clcontext_result.status().message();
  }
  std::unique_ptr<core::opencl::CLContext> clcontext_owner =
      std::move(*clcontext_result);
  core::opencl::CLContext& clcontext = *clcontext_owner;
  // print opencl info
  core::opencl::CLContext::PrintInfo();

  auto clprogram_result = core::opencl::CLProgram::Create(
      &clcontext, std::string(SHADER_PATH) + "vec_add.cl");
  if (!clprogram_result.ok()) {
    FAIL() << clprogram_result.status().message();
  }
  std::unique_ptr<core::opencl::CLProgram> clprogram_owner =
      std::move(*clprogram_result);
  core::opencl::CLProgram& clprogram = *clprogram_owner;

  auto clkernel_result = core::opencl::CLKernel::Create(&clprogram, "vec_add");
  if (!clkernel_result.ok()) {
    FAIL() << clkernel_result.status().message();
  }
  std::unique_ptr<core::opencl::CLKernel> clkernel_owner =
      std::move(*clkernel_result);
  core::opencl::CLKernel& clkernel = *clkernel_owner;

  auto clqueue_result = core::opencl::CLCommandQueue::Create(&clcontext);
  if (!clqueue_result.ok()) {
    FAIL() << clqueue_result.status().message();
  }
  std::unique_ptr<core::opencl::CLCommandQueue> clqueue_owner =
      std::move(*clqueue_result);
  core::opencl::CLCommandQueue& clqueue = *clqueue_owner;

  auto buffer_a_result = core::opencl::CLBuffer::Create(
      &clcontext, buffer_size, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
      A.data());
  if (!buffer_a_result.ok()) {
    FAIL() << buffer_a_result.status().message();
  }
  std::unique_ptr<core::opencl::CLBuffer> buffer_a_owner =
      std::move(*buffer_a_result);
  core::opencl::CLBuffer& buffer_a = *buffer_a_owner;

  auto buffer_b_result = core::opencl::CLBuffer::Create(
      &clcontext, buffer_size, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
      B.data());
  if (!buffer_b_result.ok()) {
    FAIL() << buffer_b_result.status().message();
  }
  std::unique_ptr<core::opencl::CLBuffer> buffer_b_owner =
      std::move(*buffer_b_result);
  core::opencl::CLBuffer& buffer_b = *buffer_b_owner;

  auto buffer_c_result = core::opencl::CLBuffer::Create(&clcontext, buffer_size,
                                                        CL_MEM_WRITE_ONLY);
  if (!buffer_c_result.ok()) {
    FAIL() << buffer_c_result.status().message();
  }
  std::unique_ptr<core::opencl::CLBuffer> buffer_c_owner =
      std::move(*buffer_c_result);
  core::opencl::CLBuffer& buffer_c = *buffer_c_owner;

  {
    const core::Status status = clkernel.SetArgs(buffer_a, buffer_b, buffer_c);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  size_t global_work_size[1] = {static_cast<size_t>(N)};
  {
    const core::Status status =
        clqueue.Submit(clkernel, 1, global_work_size, nullptr, nullptr);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = clqueue.Finish();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status =
        clqueue.ReadBuffer(buffer_c, C.data(), buffer_size);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  for (int i = 0; i < N; ++i) {
    std::cout << "C[" << i << "] = " << C[i] << '\n';
  }
}

TEST(OpenCL, GaussianBlur) {
  // Prepare input and fill with random data
  core::Mat<float, 1> src(4000, 3000);
  src.Random();

  // Initialize OpenCL
  int init = core::opencl::cl_init();
  if (init) {
    FAIL() << core::Status(core::ErrorCode::kBackend,
                           "Failed to initialize OpenCL loader")
                  .message();
  }

  auto clcontext_result = core::opencl::CLContext::Create();
  if (!clcontext_result.ok()) {
    FAIL() << clcontext_result.status().message();
  }
  std::unique_ptr<core::opencl::CLContext> clcontext_owner =
      std::move(*clcontext_result);
  core::opencl::CLContext& clcontext = *clcontext_owner;

  auto clprogram_result = core::opencl::CLProgram::Create(
      &clcontext, std::string(SHADER_PATH) + "gaussian_blur.cl");
  if (!clprogram_result.ok()) {
    FAIL() << clprogram_result.status().message();
  }
  std::unique_ptr<core::opencl::CLProgram> clprogram_owner =
      std::move(*clprogram_result);
  core::opencl::CLProgram& clprogram = *clprogram_owner;

  auto clkernel_result =
      core::opencl::CLKernel::Create(&clprogram, "gaussian_blur");
  if (!clkernel_result.ok()) {
    FAIL() << clkernel_result.status().message();
  }
  std::unique_ptr<core::opencl::CLKernel> clkernel_owner =
      std::move(*clkernel_result);
  core::opencl::CLKernel& clkernel = *clkernel_owner;

  auto clqueue_result = core::opencl::CLCommandQueue::Create(
      &clcontext, CL_QUEUE_PROFILING_ENABLE);
  if (!clqueue_result.ok()) {
    FAIL() << clqueue_result.status().message();
  }
  std::unique_ptr<core::opencl::CLCommandQueue> clqueue_owner =
      std::move(*clqueue_result);
  core::opencl::CLCommandQueue& clqueue = *clqueue_owner;

  size_t src_size = src.rows() * src.cols() * sizeof(float);

  auto input_buffer_result = core::opencl::CLBuffer::Create(
      &clcontext, src_size, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
      src.data());
  if (!input_buffer_result.ok()) {
    FAIL() << input_buffer_result.status().message();
  }
  std::unique_ptr<core::opencl::CLBuffer> input_buffer_owner =
      std::move(*input_buffer_result);
  core::opencl::CLBuffer& input_buffer = *input_buffer_owner;

  auto output_buffer_result =
      core::opencl::CLBuffer::Create(&clcontext, src_size, CL_MEM_WRITE_ONLY);
  if (!output_buffer_result.ok()) {
    FAIL() << output_buffer_result.status().message();
  }
  std::unique_ptr<core::opencl::CLBuffer> output_buffer_owner =
      std::move(*output_buffer_result);
  core::opencl::CLBuffer& output_buffer = *output_buffer_owner;

  // Set kernel arguments
  const int radius = 1;      // 3x3 kernel
  const float sigma = 2.0f;  // match GPU and CPU
  {
    const core::Status status = clkernel.SetArgs(
        input_buffer, output_buffer, src.cols(), src.rows(), radius, sigma);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  cl_event event = nullptr;
  // Enqueue kernel
  size_t global_work_size[2] = {static_cast<size_t>(src.cols()), static_cast<size_t>(src.rows())};
  {
    const core::Status status =
        clqueue.Submit(clkernel, 2, global_work_size, nullptr, &event);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  clWaitForEvents(1, &event);

  cl_ulong start = 0, end = 0;
  clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start), &start, nullptr);
  clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end), &end, nullptr);
  const double ms = (end - start) * 1e-6;
  // TODO: check the gpu time, 0.5ms is too fast for 4000x3000 image
  printf("Kernel took %.3f ms\n", ms);

  // Read back output image
  core::Mat<float, 1> dst(src.rows(), src.cols());
  {
    const core::Status status =
        clqueue.ReadBuffer(output_buffer, dst.data(), src_size);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  // CPU reference implementation (3x3 Gaussian with clamping, same sigma)
  auto gaussian = [](float x, float s) -> float { return std::exp(-(x * x) / (2.0f * s * s)); };
  auto clampi = [](int v, int lo, int hi) -> int { return v < lo ? lo : (v > hi ? hi : v); };

  core::Mat<float, 1> ref(src.rows(), src.cols());
  const int width = src.cols();
  const int height = src.rows();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float accum = 0.0f;
      float wsum = 0.0f;
      for (int dy = -radius; dy <= radius; ++dy) {
        const int yy = clampi(y + dy, 0, height - 1);
        const float wy = gaussian(static_cast<float>(dy), sigma);
        for (int dx = -radius; dx <= radius; ++dx) {
          const int xx = clampi(x + dx, 0, width - 1);
          const float wx = gaussian(static_cast<float>(dx), sigma);
          const float w = wx * wy;
          accum += src(yy, xx)[0] * w;
          wsum += w;
        }
      }
      ref(y, x)[0] = (wsum > 0.0f) ? (accum / wsum) : src(y, x)[0];
    }
  }

  // Compare GPU and CPU results
  double max_abs_err = 0.0;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const double a = static_cast<double>(dst(y, x)[0]);
      const double b = static_cast<double>(ref(y, x)[0]);
      const double e = std::abs(a - b);
      if (e > max_abs_err) max_abs_err = e;
    }
  }
  std::cout << "GaussianBlur CPU vs GPU max abs error: " << max_abs_err << std::endl;
  ASSERT_LT(max_abs_err, 1e-3) << "CPU and GPU Gaussian blur differ too much";

  clReleaseEvent(event);
}

TEST(OpenCL, MapMemGaussianBlur) {
  const int width = 4000;
  const int height = 3000;
  // Initialize OpenCL
  int init = core::opencl::cl_init();
  if (init) {
    FAIL() << core::Status(core::ErrorCode::kBackend,
                           "Failed to initialize OpenCL loader")
                  .message();
  }

  auto clcontext_result = core::opencl::CLContext::Create();
  if (!clcontext_result.ok()) {
    FAIL() << clcontext_result.status().message();
  }
  std::unique_ptr<core::opencl::CLContext> clcontext_owner =
      std::move(*clcontext_result);
  core::opencl::CLContext& clcontext = *clcontext_owner;

  auto clprogram_result = core::opencl::CLProgram::Create(
      &clcontext, std::string(SHADER_PATH) + "gaussian_blur.cl");
  if (!clprogram_result.ok()) {
    FAIL() << clprogram_result.status().message();
  }
  std::unique_ptr<core::opencl::CLProgram> clprogram_owner =
      std::move(*clprogram_result);
  core::opencl::CLProgram& clprogram = *clprogram_owner;

  auto clkernel_result =
      core::opencl::CLKernel::Create(&clprogram, "gaussian_blur");
  if (!clkernel_result.ok()) {
    FAIL() << clkernel_result.status().message();
  }
  std::unique_ptr<core::opencl::CLKernel> clkernel_owner =
      std::move(*clkernel_result);
  core::opencl::CLKernel& clkernel = *clkernel_owner;

  auto clqueue_result = core::opencl::CLCommandQueue::Create(&clcontext);
  if (!clqueue_result.ok()) {
    FAIL() << clqueue_result.status().message();
  }
  std::unique_ptr<core::opencl::CLCommandQueue> clqueue_owner =
      std::move(*clqueue_result);
  core::opencl::CLCommandQueue& clqueue = *clqueue_owner;

  const size_t src_size = static_cast<size_t>(height) * static_cast<size_t>(width) * sizeof(float);

  auto input_buffer_result = core::opencl::CLBuffer::Create(
      &clcontext, src_size, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR);
  if (!input_buffer_result.ok()) {
    FAIL() << input_buffer_result.status().message();
  }
  std::unique_ptr<core::opencl::CLBuffer> input_buffer_owner =
      std::move(*input_buffer_result);
  core::opencl::CLBuffer& input_buffer = *input_buffer_owner;

  auto output_buffer_result = core::opencl::CLBuffer::Create(
      &clcontext, src_size, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR);
  if (!output_buffer_result.ok()) {
    FAIL() << output_buffer_result.status().message();
  }
  std::unique_ptr<core::opencl::CLBuffer> output_buffer_owner =
      std::move(*output_buffer_result);
  core::opencl::CLBuffer& output_buffer = *output_buffer_owner;

  auto mapped_input_result =
      input_buffer.MapBuffer<float>(clqueue.queue, CL_MAP_WRITE);
  if (!mapped_input_result.ok()) {
    FAIL() << mapped_input_result.status().message();
  }
  float* mapped_input = std::move(*mapped_input_result);
  core::MatView<float, 1> input_view(mapped_input, height, width);

  // Prepare input while the OpenCL buffer is mapped to host memory.
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      input_view(y, x)[0] = dist(gen);
    }
  }

  // Compute the reference while the mapped input pointer is valid.
  const int radius = 1;      // 3x3 kernel
  const float sigma = 2.0f;  // match GPU and CPU
  auto gaussian = [](float x, float s) -> float { return std::exp(-(x * x) / (2.0f * s * s)); };
  auto clampi = [](int v, int lo, int hi) -> int { return v < lo ? lo : (v > hi ? hi : v); };

  core::Mat<float, 1> ref(height, width);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float accum = 0.0f;
      float wsum = 0.0f;
      for (int dy = -radius; dy <= radius; ++dy) {
        const int yy = clampi(y + dy, 0, height - 1);
        const float wy = gaussian(static_cast<float>(dy), sigma);
        for (int dx = -radius; dx <= radius; ++dx) {
          const int xx = clampi(x + dx, 0, width - 1);
          const float wx = gaussian(static_cast<float>(dx), sigma);
          const float w = wx * wy;
          accum += input_view(yy, xx)[0] * w;
          wsum += w;
        }
      }
      ref(y, x)[0] = (wsum > 0.0f) ? (accum / wsum) : input_view(y, x)[0];
    }
  }

  {
    const core::Status status =
        input_buffer.UnmapBuffer(clqueue.queue, mapped_input);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = clkernel.SetArgs(input_buffer, output_buffer,
                                                 width, height, radius, sigma);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  size_t global_work_size[2] = {static_cast<size_t>(width), static_cast<size_t>(height)};
  {
    const core::Status status = clqueue.Submit(clkernel, 2, global_work_size);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  {
    const core::Status status = clqueue.Finish();
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }

  auto mapped_output_result =
      output_buffer.MapBuffer<float>(clqueue.queue, CL_MAP_READ);
  if (!mapped_output_result.ok()) {
    FAIL() << mapped_output_result.status().message();
  }
  float* mapped_output = std::move(*mapped_output_result);
  core::MatView<float, 1> output_view(mapped_output, height, width);

  // Compare GPU and CPU results
  double max_abs_err = 0.0;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const double a = static_cast<double>(output_view(y, x)[0]);
      const double b = static_cast<double>(ref(y, x)[0]);
      const double e = std::abs(a - b);
      if (e > max_abs_err) max_abs_err = e;
    }
  }
  {
    const core::Status status =
        output_buffer.UnmapBuffer(clqueue.queue, mapped_output);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
  std::cout << "GaussianBlur CPU vs GPU max abs error: " << max_abs_err << std::endl;
  ASSERT_LT(max_abs_err, 1e-3) << "CPU and GPU Gaussian blur differ too much";
}

}  // namespace test
}  // namespace core
