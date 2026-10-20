#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <utility>

#include "CLBuffer.h"
#include "CLCommandQueue.h"
#include "CLContext.h"
#include "CLKernel.h"
#include "CLProgram.h"

namespace core::test {
namespace {

template <typename T>
class ScopedSymbol {
 public:
  ScopedSymbol(T& symbol, T replacement) : symbol_(symbol), previous_(symbol) {
    symbol_ = replacement;
  }
  ~ScopedSymbol() { symbol_ = previous_; }
  ScopedSymbol(const ScopedSymbol&) = delete;
  ScopedSymbol& operator=(const ScopedSymbol&) = delete;

 private:
  T& symbol_;
  T previous_;
};

int released_programs = 0;

cl_program CL_API_CALL CreateProgram(cl_context, cl_uint, const char**,
                                     const size_t*, cl_int* error) {
  *error = CL_SUCCESS;
  return reinterpret_cast<cl_program>(std::uintptr_t{1});
}

cl_int CL_API_CALL BuildProgram(cl_program, cl_uint, const cl_device_id*,
                                const char*,
                                void(CL_CALLBACK*)(cl_program, void*), void*) {
  return CL_BUILD_PROGRAM_FAILURE;
}

cl_int CL_API_CALL GetBuildInfo(cl_program, cl_device_id, cl_program_build_info,
                                size_t size, void* value,
                                size_t* returned_size) {
  constexpr char kLog[] = "stub compiler diagnostic";
  if (returned_size != nullptr) {
    *returned_size = sizeof(kLog);
  }
  if (value != nullptr && size >= sizeof(kLog)) {
    std::memcpy(value, kLog, sizeof(kLog));
  }
  return CL_SUCCESS;
}

cl_int CL_API_CALL ReleaseProgram(cl_program) {
  ++released_programs;
  return CL_SUCCESS;
}

TEST(CLStatusTest, InvalidParentsFailWithoutAllocating) {
  EXPECT_EQ(opencl::CLBuffer::Create(nullptr, 16).status().code(),
            ErrorCode::kInvalidArgument);
  EXPECT_EQ(opencl::CLCommandQueue::Create(nullptr).status().code(),
            ErrorCode::kInvalidArgument);
  EXPECT_EQ(opencl::CLKernel::Create(nullptr, "kernel").status().code(),
            ErrorCode::kInvalidArgument);
  EXPECT_EQ(opencl::CLProgram::Create(nullptr, std::span<const unsigned char>())
                .status()
                .code(),
            ErrorCode::kInvalidArgument);
}

TEST(CLStatusTest, FailedBuildPreservesLogAndReleasesProgramExactlyOnce) {
  ScopedSymbol create(clCreateProgramWithSource, &CreateProgram);
  ScopedSymbol build(clBuildProgram, &BuildProgram);
  ScopedSymbol info(clGetProgramBuildInfo, &GetBuildInfo);
  ScopedSymbol release(clReleaseProgram, &ReleaseProgram);
  opencl::CLContext context;
  context.context = reinterpret_cast<cl_context>(std::uintptr_t{1});
  context.device = reinterpret_cast<cl_device_id>(std::uintptr_t{1});
  released_programs = 0;
#if defined(__ANDROID__)
  constexpr char kSourcePath[] =
      "/data/local/tmp/core/tests/shaders/vec_add.cl";
#else
  constexpr char kSourcePath[] = "tests/shaders/vec_add.cl";
#endif
  const auto result =
      opencl::CLProgram::Create(&context, std::string(kSourcePath));
  // The test borrows a fake context handle; CLContext must not release it.
  context.context = nullptr;
  ASSERT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), ErrorCode::kBackend);
  EXPECT_EQ(result.status().native_code(), CL_BUILD_PROGRAM_FAILURE);
  EXPECT_NE(result.status().message().find("stub compiler diagnostic"),
            std::string::npos);
  EXPECT_EQ(released_programs, 1);
}

}  // namespace
}  // namespace core::test
