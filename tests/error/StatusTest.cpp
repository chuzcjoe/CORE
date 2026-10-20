#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

#include "Status.h"

namespace core {
namespace {

TEST(StatusTest, DefaultIsSuccess) {
  const Status status;
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(status.code(), ErrorCode::kOk);
  EXPECT_FALSE(status.native_code().has_value());
}

TEST(StatusTest, PreservesDiagnostics) {
  const Status status(ErrorCode::kBackend, "Device unavailable", "Vulkan", -4);
  EXPECT_FALSE(status.ok());
  EXPECT_EQ(status.message(), "Device unavailable");
  EXPECT_EQ(status.backend(), "Vulkan");
  ASSERT_TRUE(status.native_code().has_value());
  EXPECT_EQ(*status.native_code(), -4);
}

TEST(ResultTest, HoldsAndMovesAnOwnedValue) {
  static_assert(!std::is_copy_constructible_v<Result<std::unique_ptr<int>>>);
  static_assert(!std::is_convertible_v<Status, Result<int>>);
  static_assert(!std::is_convertible_v<int, Result<int>>);
  Result<std::unique_ptr<int>> result(std::make_unique<int>(42));
  ASSERT_TRUE(result.ok());
  EXPECT_TRUE(result.status().ok());
  Result<std::unique_ptr<int>> moved(std::move(result));
  EXPECT_EQ(**moved, 42);
  std::unique_ptr<int> value = *std::move(moved);
  EXPECT_EQ(*value, 42);
}

TEST(ResultTest, PreservesFailure) {
  const Result<int> result(Status(ErrorCode::kOutOfRange, "Invalid offset"));
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), ErrorCode::kOutOfRange);
  EXPECT_EQ(result.status().message(), "Invalid offset");
}

#if GTEST_HAS_DEATH_TEST
TEST(ResultTest, InvalidAccessTerminatesInReleaseBuilds) {
  const Result<int> result(Status(ErrorCode::kInvalidState, "No value"));
  EXPECT_DEATH(static_cast<void>(*result), "No value");
  EXPECT_DEATH(static_cast<void>(result.operator->()), "No value");
}

TEST(ResultTest, SuccessStatusRequiresAValue) {
  EXPECT_DEATH(static_cast<void>(Result<int>(Status())), "requires a value");
}
#endif

}  // namespace
}  // namespace core
