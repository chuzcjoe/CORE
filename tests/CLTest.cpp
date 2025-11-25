#include <gtest/gtest.h>

#include <iostream>

#include "CLBuffer.h"
#include "CLCommandQueue.h"
#include "CLContext.h"
#include "CLKernel.h"
#include "CLLoader.h"
#include "CLProgram.h"

namespace core {
namespace test {

TEST(OpenCL, test1) {
  int init = core::opencl::cl_init();
  if (init) {
    throw std::runtime_error("Failed to initialize OpenCL loader");
  }

  cl_int error = 0;
  cl_platform_id platform_ids[10];
  cl_uint num_platforms;
  error = clGetPlatformIDs(10, platform_ids, &num_platforms);
  if (error != CL_SUCCESS) {
    throw std::runtime_error("clGetPlatformIDs failed");
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
    throw std::runtime_error("Failed to initialize OpenCL loader");
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
    throw std::runtime_error("clBuildProgram failed:\n" + std::string(log.data()));
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
    throw std::runtime_error("Failed to initialize OpenCL loader");
  }

  size_t globalWorkSize = N;
  size_t buffer_size = sizeof(float) * N;

  core::opencl::CLContext clcontext;
  // print opencl info
  core::opencl::CLContext::PrintInfo();

  core::opencl::CLProgram clprogram(&clcontext, "./tests/shaders/vec_add.cl");
  core::opencl::CLKernel clkernel(&clprogram, "vec_add");
  core::opencl::CLCommandQueue clqueue(&clcontext);
  core::opencl::CLBuffer buffer_a(&clcontext, buffer_size, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                  A.data());
  core::opencl::CLBuffer buffer_b(&clcontext, buffer_size, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                  B.data());
  core::opencl::CLBuffer buffer_c(&clcontext, buffer_size, CL_MEM_WRITE_ONLY);

  clkernel.SetArgs(buffer_a, buffer_b, buffer_c);

  clqueue.Submit(clkernel, globalWorkSize);
  clqueue.Finish();
  clqueue.ReadBuffer(buffer_c, C.data(), buffer_size);

  for (int i = 0; i < N; ++i) {
    std::cout << "C[" << i << "] = " << C[i] << '\n';
  }
}

}  // namespace test
}  // namespace core