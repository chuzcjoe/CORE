#pragma once

#include <span>
#include <string>
#include <vector>

#include "CLContext.h"

namespace core {
namespace opencl {

class CLProgram {
 public:
  // Construct by loading a .cl file, creating the program, and building it.
  // Throws std::runtime_error on failure with build log if available.
  CLProgram(CLContext* context, const std::string& source_path,
            const char* build_options = nullptr);
  // Load a device/driver-specific binary, then build its executable. The bytes
  // are consumed during construction; context must outlive this object.
  CLProgram(CLContext* context, std::span<const unsigned char> binary,
            const char* build_options = nullptr);
  ~CLProgram();

  // Return an owned copy of the built program's single-device binary.
  // Throws std::runtime_error if the driver cannot provide a binary.
  std::vector<unsigned char> GetBinary() const;

  cl_program program = nullptr;

 private:
  CLContext* context_ = nullptr;
  static std::string ReadFile(const std::string& path);
  void Build(const char* build_options);
};

}  // namespace opencl
}  // namespace core
