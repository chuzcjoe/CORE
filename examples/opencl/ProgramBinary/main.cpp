#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "CLBuffer.h"
#include "CLCommandQueue.h"
#include "CLContext.h"
#include "CLKernel.h"
#include "CLLoader.h"
#include "CLProgram.h"
#include "Status.h"

namespace {

core::Status RunVectorAdd(core::opencl::CLContext* context,
                          core::opencl::CLProgram* program) {
  constexpr size_t kCount = 257;
  constexpr size_t kBytes = kCount * sizeof(float);
  std::vector<float> a(kCount), b(kCount), output(kCount);
  for (size_t i = 0; i < kCount; ++i) {
    a[i] = static_cast<float>(i);
    b[i] = static_cast<float>(i * 10U);
  }

  auto src_a_result = core::opencl::CLBuffer::Create(
      context, kBytes, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, a.data());
  if (!src_a_result.ok()) {
    return src_a_result.status();
  }
  std::unique_ptr<core::opencl::CLBuffer> src_a_owner =
      std::move(*src_a_result);
  core::opencl::CLBuffer& src_a = *src_a_owner;

  auto src_b_result = core::opencl::CLBuffer::Create(
      context, kBytes, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, b.data());
  if (!src_b_result.ok()) {
    return src_b_result.status();
  }
  std::unique_ptr<core::opencl::CLBuffer> src_b_owner =
      std::move(*src_b_result);
  core::opencl::CLBuffer& src_b = *src_b_owner;

  auto dst_result =
      core::opencl::CLBuffer::Create(context, kBytes, CL_MEM_WRITE_ONLY);
  if (!dst_result.ok()) {
    return dst_result.status();
  }
  std::unique_ptr<core::opencl::CLBuffer> dst_owner = std::move(*dst_result);
  core::opencl::CLBuffer& dst = *dst_owner;

  auto kernel_result = core::opencl::CLKernel::Create(program, "vec_add");
  if (!kernel_result.ok()) {
    return kernel_result.status();
  }
  std::unique_ptr<core::opencl::CLKernel> kernel_owner =
      std::move(*kernel_result);
  core::opencl::CLKernel& kernel = *kernel_owner;
  {
    const core::Status status = kernel.SetArgs(src_a, src_b, dst);
    if (!status.ok()) {
      return status;
    }
  }

  auto queue_result = core::opencl::CLCommandQueue::Create(context);
  if (!queue_result.ok()) {
    return queue_result.status();
  }
  std::unique_ptr<core::opencl::CLCommandQueue> queue_owner =
      std::move(*queue_result);
  core::opencl::CLCommandQueue& queue = *queue_owner;
  {
    const core::Status status = queue.Submit(kernel, 1, &kCount);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = queue.ReadBuffer(dst, output.data(), kBytes);
    if (!status.ok()) {
      return status;
    }
  }
  for (size_t i = 0; i < kCount; ++i) {
    if (output[i] != a[i] + b[i]) {
      return core::Status(core::ErrorCode::kBackend,
                          "vec_add output mismatch at " + std::to_string(i));
    }
  }
  std::cout << "Verified " << kCount << " vector sums\n";

  return core::Status();
}

}  // namespace

core::Status RunApplication(int argc, char** argv) {
  const bool save = argc == 4 && std::string(argv[1]) == "save";
  const bool load = argc == 3 && std::string(argv[1]) == "load";
  if (!save && !load) {
    std::cerr << "Usage: cl_program_binary_demo save <vec_add.cl> <program.bin>\n"
                 "       cl_program_binary_demo load <program.bin>\n";
    return core::Status(core::ErrorCode::kBackend, "Application failed");
  }

    if (core::opencl::cl_init() != CL_SUCCESS) {
      return core::Status(core::ErrorCode::kBackend,
                          "Failed to initialize OpenCL loader");
    }

    auto context_result = core::opencl::CLContext::Create();
    if (!context_result.ok()) {
      return context_result.status();
    }
    std::unique_ptr<core::opencl::CLContext> context_owner =
        std::move(*context_result);
    core::opencl::CLContext& context = *context_owner;
    std::unique_ptr<core::opencl::CLProgram> program;
    if (save) {
      auto program_creation =
          core::opencl::CLProgram::Create(&context, std::string(argv[2]));
      if (!program_creation.ok()) {
        return program_creation.status();
      }
      program = std::move(*program_creation);

      auto binary_result = program->GetBinary();
      if (!binary_result.ok()) {
        return binary_result.status();
      }
      const std::vector<unsigned char> binary = std::move(*binary_result);
      if (binary.size() > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) {
        return core::Status(core::ErrorCode::kBackend,
                            "Binary is too large to write");
      }
      std::ofstream output(argv[3], std::ios::binary);
      output.write(reinterpret_cast<const char*>(binary.data()),
                   static_cast<std::streamsize>(binary.size()));
      output.close();
      if (!output) {
        return core::Status(core::ErrorCode::kBackend,
                            "Failed to write program binary");
      }
      std::cout << "Saved " << binary.size() << " binary bytes\n";
    } else {
      std::ifstream input(argv[2], std::ios::binary);
      if (!input) {
        return core::Status(core::ErrorCode::kBackend,
                            "Failed to open program binary");
      }
      const std::string bytes((std::istreambuf_iterator<char>(input)),
                              std::istreambuf_iterator<char>());
      if (input.bad()) {
        return core::Status(core::ErrorCode::kBackend,
                            "Failed to read program binary");
      }
      const std::span<const unsigned char> binary(
          reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
      auto program_creation = core::opencl::CLProgram::Create(&context, binary);
      if (!program_creation.ok()) {
        return program_creation.status();
      }
      program = std::move(*program_creation);
      std::cout << "Loaded " << binary.size() << " binary bytes\n";
    }
    {
      const core::Status status = RunVectorAdd(&context, program.get());
      if (!status.ok()) {
        return status;
      }
    }

    return core::Status();
}

int main(int argc, char** argv) {
  const core::Status status = RunApplication(argc, argv);
  if (!status.ok()) {
    std::fprintf(stderr, "%s\n", status.message().c_str());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
