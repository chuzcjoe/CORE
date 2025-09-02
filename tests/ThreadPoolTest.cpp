#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

#include "ThreadPool.h"
#include "Timer.h"

namespace core {
namespace test {

// This test case compute the sum of 30000 elements in a vector using 3 tasks

TEST(ThreadPoolTest, test) {
  core::ThreadPool pool(3);
  Timer t;

  // Make 30,000 random floats
  std::vector<float> v(300000);
  std::mt19937 rng(123);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
  std::generate(v.begin(), v.end(), [&] { return dist(rng); });

  t.reset();
  t.start();
  // Split into 3 tasks of 300,000 each (use double accumulator to reduce FP error)
  auto sum1 = pool.submit([&v] { return std::accumulate(v.begin(), v.begin() + 100000, 0.0f); });
  auto sum2 =
      pool.submit([&v] { return std::accumulate(v.begin() + 100000, v.begin() + 200000, 0.0f); });
  auto sum3 = pool.submit([&v] { return std::accumulate(v.begin() + 200000, v.end(), 0.0f); });

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