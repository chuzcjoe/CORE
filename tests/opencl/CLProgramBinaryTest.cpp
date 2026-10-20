#include <gtest/gtest.h>

#include <cstddef>
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

namespace core::test {
namespace {

#if defined(__ANDROID__)
constexpr char kSourcePath[] = "/data/local/tmp/core/tests/shaders/program_binary.cl";
#else
constexpr char kSourcePath[] = "./tests/shaders/program_binary.cl";
#endif

core::Status CheckOutput(opencl::CLContext* context, opencl::CLProgram* program,
                         size_t count) {
  std::vector<cl_uint> input(count);
  std::vector<cl_uint> output(count);
  for (size_t i = 0; i < count; ++i) {
    input[i] = static_cast<cl_uint>(i);
  }
  const size_t bytes = count * sizeof(cl_uint);

  auto src_result = opencl::CLBuffer::Create(
      context, bytes, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, input.data());
  if (!src_result.ok()) {
    return src_result.status();
  }
  std::unique_ptr<opencl::CLBuffer> src_owner = std::move(*src_result);
  opencl::CLBuffer& src = *src_owner;

  auto dst_result = opencl::CLBuffer::Create(context, bytes, CL_MEM_WRITE_ONLY);
  if (!dst_result.ok()) {
    return dst_result.status();
  }
  std::unique_ptr<opencl::CLBuffer> dst_owner = std::move(*dst_result);
  opencl::CLBuffer& dst = *dst_owner;

  auto kernel_result = opencl::CLKernel::Create(program, "transform");
  if (!kernel_result.ok()) {
    return kernel_result.status();
  }
  std::unique_ptr<opencl::CLKernel> kernel_owner = std::move(*kernel_result);
  opencl::CLKernel& kernel = *kernel_owner;
  {
    const core::Status status = kernel.SetArgs(src, dst);
    if (!status.ok()) {
      return status;
    }
  }

  auto queue_result = opencl::CLCommandQueue::Create(context);
  if (!queue_result.ok()) {
    return queue_result.status();
  }
  std::unique_ptr<opencl::CLCommandQueue> queue_owner =
      std::move(*queue_result);
  opencl::CLCommandQueue& queue = *queue_owner;
  {
    const core::Status status = queue.Submit(kernel, 1, &count);
    if (!status.ok()) {
      return status;
    }
  }
  {
    const core::Status status = queue.ReadBuffer(dst, output.data(), bytes);
    if (!status.ok()) {
      return status;
    }
  }
  for (size_t i = 0; i < count; ++i) {
    EXPECT_EQ(output[i], input[i] * 7U + 17U) << "element " << i;
  }

  return core::Status();
}

TEST(CLProgramBinary, RoundTripPreservesBuildOptionsAndRunsInFreshContext) {
  ASSERT_EQ(opencl::cl_init(), CL_SUCCESS);
  std::vector<unsigned char> binary;
  {
    auto context_result = opencl::CLContext::Create();
    if (!context_result.ok()) {
      FAIL() << context_result.status().message();
    }
    std::unique_ptr<opencl::CLContext> context_owner =
        std::move(*context_result);
    opencl::CLContext& context = *context_owner;

    auto source_result = opencl::CLProgram::Create(
        &context, std::string(kSourcePath), "-DFACTOR=7");
    if (!source_result.ok()) {
      FAIL() << source_result.status().message();
    }
    std::unique_ptr<opencl::CLProgram> source_owner = std::move(*source_result);
    opencl::CLProgram& source = *source_owner;
    {
      const core::Status status = CheckOutput(&context, &source, 257U);
      if (!status.ok()) {
        FAIL() << status.message();
      }
    }

    auto binary_result = source.GetBinary();
    if (!binary_result.ok()) {
      FAIL() << binary_result.status().message();
    }
    binary = std::move(*binary_result);
  }
  ASSERT_FALSE(binary.empty());

  auto context_result = opencl::CLContext::Create();
  if (!context_result.ok()) {
    FAIL() << context_result.status().message();
  }
  std::unique_ptr<opencl::CLContext> context_owner = std::move(*context_result);
  opencl::CLContext& context = *context_owner;
  std::vector<unsigned char> exported;
  {
    auto loaded_result = opencl::CLProgram::Create(
        &context, std::span<const unsigned char>(binary));
    if (!loaded_result.ok()) {
      FAIL() << loaded_result.status().message();
    }
    std::unique_ptr<opencl::CLProgram> loaded_owner = std::move(*loaded_result);
    opencl::CLProgram& loaded = *loaded_owner;
    // Neither the source program nor its original context is still alive.
    binary.clear();
    binary.shrink_to_fit();
    for (const size_t count : {1U, 257U, 4096U}) {
      {
        const core::Status status = CheckOutput(&context, &loaded, count);
        if (!status.ok()) {
          FAIL() << status.message();
        }
      }
    }

    auto exported_result = loaded.GetBinary();
    if (!exported_result.ok()) {
      FAIL() << exported_result.status().message();
    }
    exported = std::move(*exported_result);
  }
  ASSERT_FALSE(exported.empty());

  auto reloaded_result = opencl::CLProgram::Create(
      &context, std::span<const unsigned char>(exported));
  if (!reloaded_result.ok()) {
    FAIL() << reloaded_result.status().message();
  }
  std::unique_ptr<opencl::CLProgram> reloaded_owner =
      std::move(*reloaded_result);
  opencl::CLProgram& reloaded = *reloaded_owner;
  {
    const core::Status status = CheckOutput(&context, &reloaded, 257U);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
}

TEST(CLProgramBinary, RejectsEmptyCorruptAndMissingContextInputs) {
  ASSERT_EQ(opencl::cl_init(), CL_SUCCESS);

  auto context_result = opencl::CLContext::Create();
  if (!context_result.ok()) {
    FAIL() << context_result.status().message();
  }
  std::unique_ptr<opencl::CLContext> context_owner = std::move(*context_result);
  opencl::CLContext& context = *context_owner;
  const std::span<const unsigned char> empty;
  EXPECT_EQ((opencl::CLProgram::Create(&context, empty)).status().code(),
            core::ErrorCode::kInvalidArgument);
  const unsigned char corrupt[] = {0x12, 0x34, 0x56, 0x78};
  const std::span<const unsigned char> bytes(corrupt);
  EXPECT_EQ((opencl::CLProgram::Create(nullptr, bytes)).status().code(),
            core::ErrorCode::kInvalidArgument);
  EXPECT_EQ((opencl::CLProgram::Create(&context, bytes)).status().code(),
            core::ErrorCode::kBackend);

  auto source_result = opencl::CLProgram::Create(
      &context, std::string(kSourcePath), "-DFACTOR=7");
  if (!source_result.ok()) {
    FAIL() << source_result.status().message();
  }
  std::unique_ptr<opencl::CLProgram> source_owner = std::move(*source_result);
  opencl::CLProgram& source = *source_owner;
  {
    const core::Status status = CheckOutput(&context, &source, 1U);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
}

TEST(CLProgramBinary, RejectsSourceBuildFailureAndCanLoadBinary) {
  ASSERT_EQ(opencl::cl_init(), CL_SUCCESS);

  auto context_result = opencl::CLContext::Create();
  if (!context_result.ok()) {
    FAIL() << context_result.status().message();
  }
  std::unique_ptr<opencl::CLContext> context_owner = std::move(*context_result);
  opencl::CLContext& context = *context_owner;

  auto source_result = opencl::CLProgram::Create(
      &context, std::string(kSourcePath), "-DFACTOR=7");
  if (!source_result.ok()) {
    FAIL() << source_result.status().message();
  }
  std::unique_ptr<opencl::CLProgram> source_owner = std::move(*source_result);
  opencl::CLProgram& source = *source_owner;

  auto binary_result = source.GetBinary();
  if (!binary_result.ok()) {
    FAIL() << binary_result.status().message();
  }
  const std::vector<unsigned char> binary = std::move(*binary_result);
  const std::span<const unsigned char> bytes(binary);
  EXPECT_EQ((opencl::CLProgram::Create(&context, std::string(kSourcePath),
                                       "-invalid-core-option"))
                .status()
                .code(),
            core::ErrorCode::kBackend);

  auto loaded_result = opencl::CLProgram::Create(&context, bytes);
  if (!loaded_result.ok()) {
    FAIL() << loaded_result.status().message();
  }
  std::unique_ptr<opencl::CLProgram> loaded_owner = std::move(*loaded_result);
  opencl::CLProgram& loaded = *loaded_owner;
  {
    const core::Status status = CheckOutput(&context, &loaded, 257U);
    if (!status.ok()) {
      FAIL() << status.message();
    }
  }
}

}  // namespace
}  // namespace core::test
