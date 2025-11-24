#pragma once

#include <vector>

#include "CLLoader.h"

namespace core {
namespace opencl {

class CLContext {
 public:
  CLContext();
  ~CLContext();

  cl_context context = nullptr;
  cl_platform_id platform = nullptr;
  cl_device_id device = nullptr;

 private:
  void QueryPlatforms();
  void QueryDevices();
};

}  // namespace opencl
}  // namespace core