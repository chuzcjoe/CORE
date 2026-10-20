#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

#include "CLContext.h"
#include "Status.h"

namespace core {
namespace opencl {

class CLProgram {
 public:
  CLProgram(const CLProgram&) = delete;
  CLProgram& operator=(const CLProgram&) = delete;
  CLProgram() = default;
  // Load a .cl file, create the program, and build it.

  static Result<std::unique_ptr<CLProgram>> Create(
      CLContext* context, const std::string& source_path,
      const char* build_options = nullptr);
  Status Init(CLContext* context, const std::string& source_path,
              const char* build_options = nullptr);
  // Load a device/driver-specific binary, then build its executable. The bytes
  // are consumed during creation; context must outlive this object.

  static Result<std::unique_ptr<CLProgram>> Create(
      CLContext* context, std::span<const unsigned char> binary,
      const char* build_options = nullptr);
  Status Init(CLContext* context, std::span<const unsigned char> binary,
              const char* build_options = nullptr);
  ~CLProgram();

  // Return an owned copy of the built program's single-device binary.
  Result<std::vector<unsigned char>> GetBinary() const;

  cl_program program = nullptr;

 private:
  CLContext* context_ = nullptr;
  static Result<std::string> ReadFile(const std::string& path);
  Status Build(const char* build_options);
  bool initialized_ = false;
};

}  // namespace opencl
}  // namespace core
