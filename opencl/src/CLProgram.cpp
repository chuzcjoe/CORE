#include "CLProgram.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#include "CLLoader.h"

namespace core {
namespace opencl {

CLProgram::CLProgram(CLContext* context, const std::string& source_path, const char* build_options)
    : context_(context) {
  const std::string source = ReadFile(source_path);
  cl_int err = CL_SUCCESS;
  const char* src_ptr = source.c_str();
  const size_t src_len = source.size();
  program = clCreateProgramWithSource(context_->context, 1, &src_ptr, &src_len, &err);
  if (err != CL_SUCCESS || program == nullptr) {
    if (program != nullptr) {
      clReleaseProgram(program);
    }
    throw std::runtime_error("clCreateProgramWithSource failed");
  }
  Build(build_options);
}

CLProgram::CLProgram(CLContext* context, std::span<const unsigned char> binary,
                     const char* build_options)
    : context_(context) {
  if (context_ == nullptr || context_->context == nullptr || context_->device == nullptr) {
    throw std::invalid_argument("CLProgram binary requires a valid context and device");
  }
  if (binary.empty()) {
    throw std::invalid_argument("CLProgram binary must not be empty");
  }
  if (clCreateProgramWithBinary == nullptr) {
    throw std::runtime_error("clCreateProgramWithBinary is unavailable");
  }
  const unsigned char* bytes = binary.data();
  const size_t size = binary.size();
  cl_int binary_status = CL_SUCCESS;
  cl_int err = CL_SUCCESS;
  program = clCreateProgramWithBinary(context_->context, 1, &context_->device, &size, &bytes,
                                     &binary_status, &err);
  if (err != CL_SUCCESS || binary_status != CL_SUCCESS || program == nullptr) {
    if (program != nullptr) {
      clReleaseProgram(program);
    }
    throw std::runtime_error("clCreateProgramWithBinary failed: error=" + std::to_string(err) +
                             ", binary_status=" + std::to_string(binary_status));
  }
  Build(build_options);
}

CLProgram::~CLProgram() {
  if (program != nullptr) {
    clReleaseProgram(program);
    program = nullptr;
  }
}

void CLProgram::Build(const char* build_options) {
  std::unique_ptr<std::remove_pointer_t<cl_program>, PFN_CLRELEASEPROGRAM> program_guard(
      program, clReleaseProgram);
  const cl_int err = clBuildProgram(program, 1, &context_->device, build_options, nullptr, nullptr);
  if (err == CL_SUCCESS) {
    program_guard.release();
    return;
  }
  // Fetch build log for diagnostics
  size_t log_size = 0;
  clGetProgramBuildInfo(program, context_->device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
  std::string log;
  if (log_size > 1) {
    log.resize(log_size);
    clGetProgramBuildInfo(program, context_->device, CL_PROGRAM_BUILD_LOG, log_size, log.data(),
                          nullptr);
  }
  throw std::runtime_error(std::string("clBuildProgram failed: ") +
                           (log.empty() ? "(no log)" : log.c_str()));
}

std::vector<unsigned char> CLProgram::GetBinary() const {
  if (clGetProgramInfo == nullptr) {
    throw std::runtime_error("clGetProgramInfo is unavailable");
  }
  cl_uint device_count = 0;
  cl_int err = clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, sizeof(device_count),
                               &device_count, nullptr);
  if (err != CL_SUCCESS || device_count != 1U) {
    throw std::runtime_error("GetBinary requires a single-device program");
  }
  size_t size = 0;
  err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size), &size, nullptr);
  if (err != CL_SUCCESS || size == 0U) {
    throw std::runtime_error("CL_PROGRAM_BINARY_SIZES failed or binary is unavailable");
  }
  std::vector<unsigned char> binary(size);
  unsigned char* bytes = binary.data();
  err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(bytes), &bytes, nullptr);
  if (err != CL_SUCCESS) {
    throw std::runtime_error("CL_PROGRAM_BINARIES failed: " + std::to_string(err));
  }
  return binary;
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
