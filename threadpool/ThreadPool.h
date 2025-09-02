#pragma once
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace core {

class ThreadPool {
 public:
  explicit ThreadPool(std::size_t thread_count = std::thread::hardware_concurrency()
                                                     ? std::thread::hardware_concurrency()
                                                     : 4) {
    workers_.reserve(thread_count);
    for (std::size_t i = 0; i < thread_count; ++i) {
      workers_.emplace_back([this] { worker_loop(); });
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  ~ThreadPool() { shutdown(); }

  template <class F, class... Args>
  auto submit(F&& f, Args&&... args)
      -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
    using R = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

    auto task_fn =
        [fn = std::decay_t<F>(std::forward<F>(f)),
         tup = std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...)]() mutable -> R {
      return std::apply(std::move(fn), std::move(tup));
    };

    auto ptask = std::make_shared<std::packaged_task<R()>>(std::move(task_fn));
    std::future<R> fut = ptask->get_future();

    {
      std::lock_guard<std::mutex> lk(m_);
      if (!accepting_) throw std::runtime_error("ThreadPool: submit on a stopped pool");
      tasks_.emplace_back([ptask]() mutable { (*ptask)(); });
    }
    cv_.notify_one();
    return fut;
  }

  void wait_idle() {
    std::unique_lock<std::mutex> lk(m_);
    idle_cv_.wait(lk, [this] { return tasks_.empty() && active_ == 0; });
  }

  void shutdown() {
    std::vector<std::thread> to_join;
    {
      std::lock_guard<std::mutex> lk(m_);
      if (!accepting_) return;
      accepting_ = false;
      to_join.swap(workers_);  // move out so we can release lock while joining
    }
    cv_.notify_all();  // wake workers

    for (auto& t : to_join) {
      if (t.joinable()) t.join();
    }

    // After joins, all workers are gone; signal idle in case anyone waits.
    {
      std::lock_guard<std::mutex> lk(m_);
      if (tasks_.empty() && active_ == 0) idle_cv_.notify_all();
    }
  }

  std::size_t size() const noexcept { return workers_.size(); }

 private:
  void worker_loop() {
    for (;;) {
      std::function<void()> task;

      {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [this] { return !tasks_.empty() || !accepting_; });

        if (!accepting_ && tasks_.empty()) break;

        task = std::move(tasks_.front());
        tasks_.pop_front();
        ++active_;
      }

      try {
        task();
      } catch (...) { /* packaged_task handles exceptions */
      }

      // NEW: notify whenever the pool is idle
      {
        std::lock_guard<std::mutex> lk(m_);
        --active_;
        if (tasks_.empty() && active_ == 0) idle_cv_.notify_all();
      }
    }
  }

  mutable std::mutex m_;
  std::condition_variable cv_;
  std::condition_variable idle_cv_;
  std::deque<std::function<void()>> tasks_;
  std::vector<std::thread> workers_;

  bool accepting_ = true;   // guarded by m_
  std::size_t active_ = 0;  // guarded by m_
};

}  // namespace core