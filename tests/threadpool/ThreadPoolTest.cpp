#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

#include "Status.h"
#include "ThreadPool.h"
#include "Timer.h"

namespace core {
namespace test {

TEST(ThreadPoolTest, RejectsZeroWorkers) {
  EXPECT_EQ((core::ThreadPool::Create(0U)).status().code(),
            core::ErrorCode::kInvalidArgument);
}

TEST(ThreadPoolTest, SingleWorkerExecutesTasks) {
  auto pool_result = core::ThreadPool::Create(1U);
  if (!pool_result.ok()) {
    FAIL() << pool_result.status().message();
  }
  std::unique_ptr<core::ThreadPool> pool_owner = std::move(*pool_result);
  core::ThreadPool& pool = *pool_owner;

  auto result_result = pool.submit([] { return 42; });
  if (!result_result.ok()) {
    FAIL() << result_result.status().message();
  }
  std::future<int> result = std::move(*result_result);

  EXPECT_EQ(pool.size(), 1U);
  EXPECT_EQ(result.get(), 42);
}

TEST(ThreadPoolTest, StoppedPoolRejectsSubmissionAndDrainsAcceptedWork) {
  auto result = core::ThreadPool::Create(1U);
  ASSERT_TRUE(result.ok());
  auto& pool = **result;
  auto accepted = pool.submit([] { return 17; });
  ASSERT_TRUE(accepted.ok());
  pool.shutdown();
  EXPECT_EQ(accepted->get(), 17);
  const auto rejected = pool.submit([] {});
  EXPECT_FALSE(rejected.ok());
  EXPECT_EQ(rejected.status().code(), ErrorCode::kInvalidState);
  pool.shutdown();
}

TEST(ThreadPoolTest, TaskReturnsARecoverableFailure) {
  auto result = core::ThreadPool::Create(1U);
  ASSERT_TRUE(result.ok());
  auto completion = (*result)->submit(
      [] { return Result<int>(Status(ErrorCode::kIo, "Task read failed")); });
  ASSERT_TRUE(completion.ok());
  const Result<int> value = completion->get();
  EXPECT_FALSE(value.ok());
  EXPECT_EQ(value.status().code(), ErrorCode::kIo);
  EXPECT_EQ(value.status().message(), "Task read failed");
}

TEST(ThreadPoolTest, AcceptsVoidTasksAndMoveOnlyArguments) {
  auto result = core::ThreadPool::Create(1U);
  ASSERT_TRUE(result.ok());
  int output = 0;
  auto completion = (*result)->submit(
      [&output](std::unique_ptr<int> value) { output = *value; },
      std::make_unique<int>(23));
  ASSERT_TRUE(completion.ok());
  completion->get();
  (*result)->wait_idle();
  EXPECT_EQ(output, 23);
}

// This test case compute the sum of 30000 elements in a vector using 3 tasks

TEST(ThreadPoolTest, test) {
  auto pool_result = core::ThreadPool::Create(3);
  if (!pool_result.ok()) {
    FAIL() << pool_result.status().message();
  }
  std::unique_ptr<core::ThreadPool> pool_owner = std::move(*pool_result);
  core::ThreadPool& pool = *pool_owner;
  Timer t;

  // Make 30,000 random floats
  std::vector<float> v(300000);
  std::mt19937 rng(123);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
  std::generate(v.begin(), v.end(), [&] { return dist(rng); });

  t.reset();
  t.start();

  auto sum1_result = pool.submit(
      [&v] { return std::accumulate(v.begin(), v.begin() + 100000, 0.0f); });
  if (!sum1_result.ok()) {
    FAIL() << sum1_result.status().message();
  }
  auto sum1 = std::move(*sum1_result);

  auto sum2_result = pool.submit([&v] {
    return std::accumulate(v.begin() + 100000, v.begin() + 200000, 0.0f);
  });
  if (!sum2_result.ok()) {
    FAIL() << sum2_result.status().message();
  }
  auto sum2 = std::move(*sum2_result);

  auto sum3_result = pool.submit(
      [&v] { return std::accumulate(v.begin() + 200000, v.end(), 0.0f); });
  if (!sum3_result.ok()) {
    FAIL() << sum3_result.status().message();
  }
  auto sum3 = std::move(*sum3_result);

  // Combine results
  const float total = sum1.get() + sum2.get() + sum3.get();
  t.end();
  printf("Using thread pool : %fms\n", t.time());

  // verify against single-thread sum
  t.reset();
  t.start();
  const float check = std::accumulate(v.begin(), v.end(), 0.0);
  t.end();
  printf("Running on a single thread: %fms\n", t.time());

  EXPECT_NEAR(total, check, 0.001f);
}

}  // namespace test
}  // namespace core
