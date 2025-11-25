#include "CLContext.h"

#include <iostream>
#include <string>

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

// Implementation of PrintInfo
namespace core {
namespace opencl {

static std::string GetPlatformInfoString(cl_platform_id pid, cl_platform_info param) {
  size_t size = 0;
  clGetPlatformInfo(pid, param, 0, nullptr, &size);
  std::string s;
  s.resize(size ? size : 1);
  if (size) {
    clGetPlatformInfo(pid, param, size, s.data(), nullptr);
    if (!s.empty() && s.back() == '\0') s.pop_back();
  }
  return s;
}

static std::string GetDeviceInfoString(cl_device_id did, cl_device_info param) {
  size_t size = 0;
  clGetDeviceInfo(did, param, 0, nullptr, &size);
  std::string s;
  s.resize(size ? size : 1);
  if (size) {
    clGetDeviceInfo(did, param, size, s.data(), nullptr);
    if (!s.empty() && s.back() == '\0') s.pop_back();
  }
  return s;
}

static std::string DeviceTypeToString(cl_device_type t) {
  std::string r;
  if (t & CL_DEVICE_TYPE_CPU) r += (r.empty() ? "" : "|") + std::string("CPU");
  if (t & CL_DEVICE_TYPE_GPU) r += (r.empty() ? "" : "|") + std::string("GPU");
  if (t & CL_DEVICE_TYPE_ACCELERATOR) r += (r.empty() ? "" : "|") + std::string("ACCEL");
  if (t & CL_DEVICE_TYPE_DEFAULT) r += (r.empty() ? "" : "|") + std::string("DEFAULT");
#ifdef CL_DEVICE_TYPE_CUSTOM
  if (t & CL_DEVICE_TYPE_CUSTOM) r += (r.empty() ? "" : "|") + std::string("CUSTOM");
#endif
  return r.empty() ? std::string("UNKNOWN") : r;
}

void CLContext::PrintInfo() {
  // Ensure loader is initialized
  cl_init();

  cl_uint num_platforms = 0;
  cl_int err = clGetPlatformIDs(0, nullptr, &num_platforms);
  if (err != CL_SUCCESS || num_platforms == 0) {
    std::cout << "No OpenCL platforms found (err=" << err << ")\n";
    return;
  }
  std::vector<cl_platform_id> platforms(num_platforms);
  clGetPlatformIDs(num_platforms, platforms.data(), nullptr);

  std::cout << "OpenCL Platforms: " << num_platforms << "\n";
  for (cl_uint i = 0; i < num_platforms; ++i) {
    auto pid = platforms[i];
    std::cout << "\n[Platform " << i << "]\n";
    std::cout << "  Name      : " << GetPlatformInfoString(pid, CL_PLATFORM_NAME) << "\n";
    std::cout << "  Vendor    : " << GetPlatformInfoString(pid, CL_PLATFORM_VENDOR) << "\n";
    std::cout << "  Version   : " << GetPlatformInfoString(pid, CL_PLATFORM_VERSION) << "\n";
    std::cout << "  Profile   : " << GetPlatformInfoString(pid, CL_PLATFORM_PROFILE) << "\n";

    // Devices
    cl_uint num_devices = 0;
    clGetDeviceIDs(pid, CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices);
    std::cout << "  Devices   : " << num_devices << "\n";
    if (num_devices == 0) continue;
    std::vector<cl_device_id> devices(num_devices);
    clGetDeviceIDs(pid, CL_DEVICE_TYPE_ALL, num_devices, devices.data(), nullptr);

    for (cl_uint d = 0; d < num_devices; ++d) {
      auto did = devices[d];
      std::cout << "    - [Device " << d << "]\n";
      std::cout << "      Name           : " << GetDeviceInfoString(did, CL_DEVICE_NAME) << "\n";
      std::cout << "      Vendor         : " << GetDeviceInfoString(did, CL_DEVICE_VENDOR) << "\n";
      std::cout << "      Driver Version : " << GetDeviceInfoString(did, CL_DRIVER_VERSION) << "\n";
      std::cout << "      Device Version : " << GetDeviceInfoString(did, CL_DEVICE_VERSION) << "\n";

      cl_device_type dtype = 0;
      clGetDeviceInfo(did, CL_DEVICE_TYPE, sizeof(dtype), &dtype, nullptr);
      std::cout << "      Type           : " << DeviceTypeToString(dtype) << "\n";

      cl_uint compute_units = 0;
      clGetDeviceInfo(did, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units,
                      nullptr);
      std::cout << "      Compute Units  : " << compute_units << "\n";

      size_t max_wg = 0;
      clGetDeviceInfo(did, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_wg), &max_wg, nullptr);
      std::cout << "      Max WG Size    : " << max_wg << "\n";

      cl_ulong global_mem = 0;
      clGetDeviceInfo(did, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem), &global_mem, nullptr);
      std::cout << "      Global Mem     : " << static_cast<unsigned long long>(global_mem)
                << " bytes\n";

      cl_ulong local_mem = 0;
      clGetDeviceInfo(did, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem), &local_mem, nullptr);
      std::cout << "      Local Mem      : " << static_cast<unsigned long long>(local_mem)
                << " bytes\n";

      cl_uint clock = 0;
      clGetDeviceInfo(did, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock), &clock, nullptr);
      std::cout << "      Max Clock      : " << clock << " MHz\n";

      cl_uint addr_bits = 0;
      clGetDeviceInfo(did, CL_DEVICE_ADDRESS_BITS, sizeof(addr_bits), &addr_bits, nullptr);
      std::cout << "      Address Bits   : " << addr_bits << "\n";

      cl_bool img_support = CL_FALSE;
      clGetDeviceInfo(did, CL_DEVICE_IMAGE_SUPPORT, sizeof(img_support), &img_support, nullptr);
      std::cout << "      Image Support  : " << (img_support ? "Yes" : "No") << "\n";
    }
  }
}

}  // namespace opencl
}  // namespace core
