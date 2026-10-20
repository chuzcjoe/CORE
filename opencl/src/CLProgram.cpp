#include "CLProgram.h"

#include <fstream>
#include <sstream>

#include "CLLoader.h"

namespace core {
namespace opencl {

Result<std::unique_ptr<CLProgram>> CLProgram::Create(
    CLContext* context, const std::string& source_path,
    const char* build_options) {
  std::unique_ptr<CLProgram> object(new CLProgram());
  const Status status = object->Init(context, source_path, build_options);
  if (!status.ok()) {
    return Result<std::unique_ptr<CLProgram>>(status);
  }
  return Result<std::unique_ptr<CLProgram>>(std::move(object));
}

Status CLProgram::Init(CLContext* context, const std::string& source_path,
                       const char* build_options) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "CLProgram initialization is single-use");
  }
  initialized_ = true;
  context_ = context;

  const Result<std::string> source_result = ReadFile(source_path);
  if (!source_result.ok()) {
    return source_result.status();
  }
  const std::string& source = *source_result;
  if (context_ == nullptr || context_->context == nullptr) {
    return Status(ErrorCode::kInvalidArgument,
                  "A live OpenCL context is required");
  }
  cl_int err = CL_SUCCESS;
  const char* src_ptr = source.c_str();
  const size_t src_len = source.size();
  program = clCreateProgramWithSource(context_->context, 1, &src_ptr, &src_len, &err);
  if (err != CL_SUCCESS || program == nullptr) {
    return Status(ErrorCode::kBackend, "clCreateProgramWithSource failed",
                  "OpenCL", err);
  }
  return Build(build_options);
}

Result<std::unique_ptr<CLProgram>> CLProgram::Create(
    CLContext* context, std::span<const unsigned char> binary,
    const char* build_options) {
  std::unique_ptr<CLProgram> object(new CLProgram());
  const Status status = object->Init(context, binary, build_options);
  if (!status.ok()) {
    return Result<std::unique_ptr<CLProgram>>(status);
  }
  return Result<std::unique_ptr<CLProgram>>(std::move(object));
}

Status CLProgram::Init(CLContext* context,
                       std::span<const unsigned char> binary,
                       const char* build_options) {
  if (initialized_) {
    return Status(ErrorCode::kInvalidState,
                  "CLProgram initialization is single-use");
  }
  initialized_ = true;
  context_ = context;

  if (context_ == nullptr || context_->context == nullptr || context_->device == nullptr) {
    return Status(ErrorCode::kInvalidArgument,
                  "CLProgram binary requires a valid context and device");
  }
  if (binary.empty()) {
    return Status(ErrorCode::kInvalidArgument,
                  "CLProgram binary must not be empty");
  }
  if (clCreateProgramWithBinary == nullptr) {
    return Status(ErrorCode::kBackend,
                  "clCreateProgramWithBinary is unavailable");
  }
  const unsigned char* bytes = binary.data();
  const size_t size = binary.size();
  cl_int binary_status = CL_SUCCESS;
  if (context_ == nullptr || context_->context == nullptr) {
    return Status(ErrorCode::kInvalidArgument,
                  "A live OpenCL context is required");
  }
  cl_int err = CL_SUCCESS;
  program = clCreateProgramWithBinary(context_->context, 1, &context_->device, &size, &bytes,
                                     &binary_status, &err);
  if (err != CL_SUCCESS || binary_status != CL_SUCCESS || program == nullptr) {
    return Status(
        ErrorCode::kBackend,
        "clCreateProgramWithBinary failed: error=" + std::to_string(err) +
            ", binary_status=" + std::to_string(binary_status),
        "OpenCL", err);
  }
  return Build(build_options);
}

CLProgram::~CLProgram() {
  if (program != nullptr) {
    clReleaseProgram(program);
    program = nullptr;
  }
}

Status CLProgram::Build(const char* build_options) {
  if (program == nullptr) {
    return Status(ErrorCode::kInvalidState,
                  "CLProgram requires successful initialization");
  }

  const cl_int err = clBuildProgram(program, 1, &context_->device,
                                    build_options, nullptr, nullptr);
  if (err == CL_SUCCESS) {
    return Status();
  }
  // Fetch build log for diagnostics
  size_t log_size = 0;
  clGetProgramBuildInfo(program, context_->device, CL_PROGRAM_BUILD_LOG, 0,
                        nullptr, &log_size);
  std::string log;
  if (log_size > 1) {
    log.resize(log_size);
    clGetProgramBuildInfo(program, context_->device, CL_PROGRAM_BUILD_LOG,
                          log_size, log.data(), nullptr);
  }
  return Status(ErrorCode::kBackend,
                std::string("clBuildProgram failed: ") +
                    (log.empty() ? "(no log)" : log.c_str()),
                "OpenCL", err);
}

Result<std::vector<unsigned char>> CLProgram::GetBinary() const {
  if (program == nullptr) {
    return Result<std::vector<unsigned char>>(
        Status(ErrorCode::kInvalidState,
               "CLProgram requires successful initialization"));
  }

  if (clGetProgramInfo == nullptr) {
    return Result<std::vector<unsigned char>>(
        Status(ErrorCode::kBackend, "clGetProgramInfo is unavailable"));
  }
  cl_uint device_count = 0;
  cl_int err = clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, sizeof(device_count),
                               &device_count, nullptr);
  if (err != CL_SUCCESS || device_count != 1U) {
    return Result<std::vector<unsigned char>>(
        Status(ErrorCode::kBackend,
               "GetBinary requires a single-device program", "OpenCL", err));
  }
  size_t size = 0;
  err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size), &size, nullptr);
  if (err != CL_SUCCESS || size == 0U) {
    return Result<std::vector<unsigned char>>(
        Status(ErrorCode::kBackend,
               "CL_PROGRAM_BINARY_SIZES failed or binary is unavailable",
               "OpenCL", err));
  }
  std::vector<unsigned char> binary(size);
  unsigned char* bytes = binary.data();
  err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(bytes), &bytes, nullptr);
  if (err != CL_SUCCESS) {
    return Result<std::vector<unsigned char>>(Status(
        ErrorCode::kBackend,
        "CL_PROGRAM_BINARIES failed: " + std::to_string(err), "OpenCL", err));
  }
  return Result<std::vector<unsigned char>>(std::move(binary));
}

Result<std::string> CLProgram::ReadFile(const std::string& path) {
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (!file) {
    return Result<std::string>(
        Status(ErrorCode::kIo, "Failed to open .cl file: " + path));
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  if (!file && !file.eof()) {
    return Result<std::string>(
        Status(ErrorCode::kIo, "Failed to read .cl file: " + path));
  }
  return Result<std::string>(ss.str());
}

}  // namespace opencl
}  // namespace core
