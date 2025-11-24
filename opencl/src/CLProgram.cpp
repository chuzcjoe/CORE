#include "CLProgram.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "CLLoader.h"

namespace core {
namespace opencl {

CLProgram::CLProgram(CLContext* context, const std::string& source_path, const char* build_options)
    : context_(context) {
  const std::string source = ReadFile(source_path);
  Build(source, build_options);
}

CLProgram::~CLProgram() {
  if (program != nullptr) {
    clReleaseProgram(program);
    program = nullptr;
  }
}

void CLProgram::Build(const std::string& source, const char* build_options) {
  cl_int err = CL_SUCCESS;
  const char* src_ptr = source.c_str();
  const size_t src_len = source.size();

  program = clCreateProgramWithSource(context_->context, 1, &src_ptr, &src_len, &err);
  if (err != CL_SUCCESS || !program) {
    throw std::runtime_error("clCreateProgramWithSource failed");
  }

  err = clBuildProgram(program, 1, &context_->device, build_options, nullptr, nullptr);
  if (err != CL_SUCCESS) {
    // Fetch build log for diagnostics
    size_t log_size = 0;
    clGetProgramBuildInfo(program, context_->device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
    std::string log;
    if (log_size > 1) {
      log.resize(log_size);
      clGetProgramBuildInfo(program, context_->device, CL_PROGRAM_BUILD_LOG, log_size, log.data(),
                            nullptr);
    }
    clReleaseProgram(program);
    program = nullptr;
    throw std::runtime_error(std::string("clBuildProgram failed: ") +
                             (log.empty() ? "(no log)" : log.c_str()));
  }
}

std::string CLProgram::ReadFile(const std::string& path) {
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open .cl file: " + path);
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  if (!file && !file.eof()) {
    throw std::runtime_error("Failed to read .cl file: " + path);
  }
  return ss.str();
}

}  // namespace opencl
}  // namespace core
