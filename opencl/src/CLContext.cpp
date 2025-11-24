#include "CLContext.h"

namespace core {
namespace opencl {

CLContext::CLContext() {
  QueryPlatforms();
  QueryDevices();

  cl_int err = CL_SUCCESS;
  context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
  if (err != CL_SUCCESS) {
    throw std::runtime_error("clCreateContext failed");
  }
}

CLContext::~CLContext() {
  if (context != nullptr) {
    clReleaseContext(context);
    context = nullptr;
  }
}

void CLContext::QueryPlatforms() {
  cl_uint num_platforms = 0;
  clGetPlatformIDs(0, nullptr, &num_platforms);
  std::vector<cl_platform_id> platforms(num_platforms);
  clGetPlatformIDs(num_platforms, platforms.data(), nullptr);
  platform = platforms[0];
}

void CLContext::QueryDevices() {
  cl_uint num_devices = 0;
  clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices);
  std::vector<cl_device_id> devices(num_devices);
  clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices, devices.data(), nullptr);
  device = devices[0];
}

}  // namespace opencl
}  // namespace core