#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "CLBuffer.h"
#include "CLCommandQueue.h"
#include "CLContext.h"
#include "CLKernel.h"
#include "CLLoader.h"
#include "CLProgram.h"

namespace {

void RunVectorAdd(core::opencl::CLContext* context, core::opencl::CLProgram* program) {
  constexpr size_t kCount = 257;
  constexpr size_t kBytes = kCount * sizeof(float);
  std::vector<float> a(kCount), b(kCount), output(kCount);
  for (size_t i = 0; i < kCount; ++i) {
    a[i] = static_cast<float>(i);
    b[i] = static_cast<float>(i * 10U);
  }
  core::opencl::CLBuffer src_a(context, kBytes, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, a.data());
  core::opencl::CLBuffer src_b(context, kBytes, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, b.data());
  core::opencl::CLBuffer dst(context, kBytes, CL_MEM_WRITE_ONLY);
  core::opencl::CLKernel kernel(program, "vec_add");
  kernel.SetArgs(src_a, src_b, dst);
  core::opencl::CLCommandQueue queue(context);
  queue.Submit(kernel, 1, &kCount);
  queue.ReadBuffer(dst, output.data(), kBytes);
  for (size_t i = 0; i < kCount; ++i) {
    if (output[i] != a[i] + b[i]) {
      throw std::runtime_error("vec_add output mismatch at " + std::to_string(i));
    }
  }
  std::cout << "Verified " << kCount << " vector sums\n";
}

}  // namespace

int main(int argc, char** argv) {
  const bool save = argc == 4 && std::string(argv[1]) == "save";
  const bool load = argc == 3 && std::string(argv[1]) == "load";
  if (!save && !load) {
    std::cerr << "Usage: cl_program_binary_demo save <vec_add.cl> <program.bin>\n"
                 "       cl_program_binary_demo load <program.bin>\n";
    return 1;
  }
  if (core::opencl::cl_init() != CL_SUCCESS) {
    throw std::runtime_error("Failed to initialize OpenCL loader");
  }
  core::opencl::CLContext context;
  std::unique_ptr<core::opencl::CLProgram> program;
  if (save) {
    program = std::make_unique<core::opencl::CLProgram>(&context, std::string(argv[2]));
    const std::vector<unsigned char> binary = program->GetBinary();
    if (binary.size() > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) {
      throw std::runtime_error("Binary is too large to write");
    }
    std::ofstream output(argv[3], std::ios::binary);
    output.write(reinterpret_cast<const char*>(binary.data()),
                 static_cast<std::streamsize>(binary.size()));
    output.close();
    if (!output) {
      throw std::runtime_error("Failed to write program binary");
    }
    std::cout << "Saved " << binary.size() << " binary bytes\n";
  } else {
    std::ifstream input(argv[2], std::ios::binary);
    if (!input) {
      throw std::runtime_error("Failed to open program binary");
    }
    const std::string bytes((std::istreambuf_iterator<char>(input)),
                            std::istreambuf_iterator<char>());
    if (input.bad()) {
      throw std::runtime_error("Failed to read program binary");
    }
    const std::span<const unsigned char> binary(
        reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
    program = std::make_unique<core::opencl::CLProgram>(&context, binary);
    std::cout << "Loaded " << binary.size() << " binary bytes\n";
  }
  RunVectorAdd(&context, program.get());
  return 0;
}
