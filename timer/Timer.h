#pragma once
#include <chrono>

namespace core {

struct Timer {
  using clock = std::chrono::steady_clock;

  void start() {
    t0_ = clock::now();
    running_ = true;
  }

  void end() {
    if (!running_) return;
    last_ms_ = std::chrono::duration<double, std::milli>(clock::now() - t0_).count();
    total_ms_ += last_ms_;
    running_ = false;
  }

  // Time (ms) for the most recent [start(), end()] pair.
  double time() const { return last_ms_; }

  // Cumulative time (ms) across all completed intervals.
  double total() const { return total_ms_; }

  void reset() {
    last_ms_ = total_ms_ = 0.0;
    running_ = false;
  }

 private:
  clock::time_point t0_{};
  bool running_{false};
  double last_ms_{0.0};
  double total_ms_{0.0};
};

}  // namespace core
