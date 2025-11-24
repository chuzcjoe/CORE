#pragma once

#include <string>

#include "CLContext.h"

namespace core {
namespace opencl {

class CLProgram {
 public:
  // Construct by loading a .cl file, creating the program, and building it.
  // Throws std::runtime_error on failure with build log if available.
  CLProgram(CLContext* context, const std::string& source_path,
            const char* build_options = nullptr);
  ~CLProgram();

  cl_program program = nullptr;

 private:
  CLContext* context_ = nullptr;
  static std::string ReadFile(const std::string& path);
  void Build(const std::string& source, const char* build_options);
};

}  // namespace opencl
}  // namespace core
