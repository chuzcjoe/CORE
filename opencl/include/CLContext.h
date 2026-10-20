#pragma once

#include <memory>
#include <vector>

#include "CLLoader.h"
#include "Status.h"

namespace core {
namespace opencl {

class CLContext {
 public:
  CLContext(const CLContext&) = delete;
  CLContext& operator=(const CLContext&) = delete;
  CLContext() = default;

  static Result<std::unique_ptr<CLContext>> Create();
  Status Init();
  ~CLContext();

  cl_context context = nullptr;
  cl_platform_id platform = nullptr;
  cl_device_id device = nullptr;

  // Print platforms, devices, and key OpenCL properties.
  static void PrintInfo();

 private:
  Status QueryPlatforms();
  Status QueryDevices();
  bool initialized_ = false;
};

}  // namespace opencl
}  // namespace core
