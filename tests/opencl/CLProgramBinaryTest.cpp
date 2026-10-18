#include "CLProgram.h"

#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "CLBuffer.h"
#include "CLCommandQueue.h"
#include "CLContext.h"
#include "CLKernel.h"
#include "CLLoader.h"

namespace core::test {
namespace {

#if defined(__ANDROID__)
constexpr char kSourcePath[] = "/data/local/tmp/core/tests/shaders/program_binary.cl";
#else
constexpr char kSourcePath[] = "./tests/shaders/program_binary.cl";
#endif

void CheckOutput(opencl::CLContext* context, opencl::CLProgram* program, size_t count) {
  std::vector<cl_uint> input(count);
  std::vector<cl_uint> output(count);
  for (size_t i = 0; i < count; ++i) {
    input[i] = static_cast<cl_uint>(i);
  }
  const size_t bytes = count * sizeof(cl_uint);
  opencl::CLBuffer src(context, bytes, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, input.data());
  opencl::CLBuffer dst(context, bytes, CL_MEM_WRITE_ONLY);
  opencl::CLKernel kernel(program, "transform");
  kernel.SetArgs(src, dst);
  opencl::CLCommandQueue queue(context);
  queue.Submit(kernel, 1, &count);
  queue.ReadBuffer(dst, output.data(), bytes);
  for (size_t i = 0; i < count; ++i) {
    EXPECT_EQ(output[i], input[i] * 7U + 17U) << "element " << i;
  }
}

TEST(CLProgramBinary, RoundTripPreservesBuildOptionsAndRunsInFreshContext) {
  ASSERT_EQ(opencl::cl_init(), CL_SUCCESS);
  std::vector<unsigned char> binary;
  {
    opencl::CLContext context;
    opencl::CLProgram source(&context, std::string(kSourcePath), "-DFACTOR=7");
    CheckOutput(&context, &source, 257U);
    binary = source.GetBinary();
  }
  ASSERT_FALSE(binary.empty());
  opencl::CLContext context;
  std::vector<unsigned char> exported;
  {
    opencl::CLProgram loaded(&context, std::span<const unsigned char>(binary));
    // Neither the source program nor its original context is still alive.
    binary.clear();
    binary.shrink_to_fit();
    for (const size_t count : {1U, 257U, 4096U}) {
      CheckOutput(&context, &loaded, count);
    }
    exported = loaded.GetBinary();
  }
  ASSERT_FALSE(exported.empty());
  opencl::CLProgram reloaded(&context, std::span<const unsigned char>(exported));
  CheckOutput(&context, &reloaded, 257U);
}

TEST(CLProgramBinary, RejectsEmptyCorruptAndMissingContextInputs) {
  ASSERT_EQ(opencl::cl_init(), CL_SUCCESS);
  opencl::CLContext context;
  const std::span<const unsigned char> empty;
  EXPECT_THROW(opencl::CLProgram(&context, empty), std::invalid_argument);
  const unsigned char corrupt[] = {0x12, 0x34, 0x56, 0x78};
  const std::span<const unsigned char> bytes(corrupt);
  EXPECT_THROW(opencl::CLProgram(nullptr, bytes), std::invalid_argument);
  EXPECT_THROW(opencl::CLProgram(&context, bytes), std::runtime_error);
  // A rejected binary must not poison the context or the source path.
  opencl::CLProgram source(&context, std::string(kSourcePath), "-DFACTOR=7");
  CheckOutput(&context, &source, 1U);
}

TEST(CLProgramBinary, RejectsSourceBuildFailureAndCanLoadBinary) {
  ASSERT_EQ(opencl::cl_init(), CL_SUCCESS);
  opencl::CLContext context;
  opencl::CLProgram source(&context, std::string(kSourcePath), "-DFACTOR=7");
  const std::vector<unsigned char> binary = source.GetBinary();
  const std::span<const unsigned char> bytes(binary);
  EXPECT_THROW(opencl::CLProgram(&context, std::string(kSourcePath), "-invalid-core-option"),
               std::runtime_error);
  opencl::CLProgram loaded(&context, bytes);
  CheckOutput(&context, &loaded, 257U);
}

}  // namespace
}  // namespace core::test
