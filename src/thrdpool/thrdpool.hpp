#pragma once
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace thrdpool {

enum class ThrdpoolState : short {
  Running = 1,
  Joining,
  Stopped,
};

using concurrency_t =
    std::invoke_result_t<decltype(std::thread::hardware_concurrency)>;

class Thrdpool {
 public:
  Thrdpool() = delete;
  Thrdpool(const concurrency_t cap)
      : capacity_(cap), state_(ThrdpoolState::Running), size_(0) {
    thrds_.reserve(cap);
    for (size_t i = 0; i < cap; i++) {
      thrds_.emplace_back(&Thrdpool::worker, this);
    }
  };

  template <typename F, typename... A>
  void PushTask(F&& task, A&&... args) {
    std::function<void()> task_handle =
        std::bind(std::forward<F>(task), std::forward<A>(args)...);
    {
      const std::scoped_lock locker(mtx_);
      taskpool_.push(std::move(task_handle));
    }
    ++size_;
    // try to wake up worker
    worker_available_cv_.notify_one();
  };

  template <
      typename F, typename... A,
      typename R = std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>>
  [[nodiscard]] std::future<R> Submit(F&& task, A&&... args) {
    std::function<R()> task_handle =
        std::bind(std::forward<F>(task), std::forward<A>(args)...);
    auto task_promise = std::make_shared<std::promise<R>>();
    PushTask([task_handle, task_promise] {
      try {
        if constexpr (std::is_void_v<R>) {
          std::invoke(task_handle);
          task_promise->set_value();
        } else {
          task_promise->set_value(std::invoke(task_handle));
        }
      } catch (...) {
        try {
          task_promise->set_exception(std::current_exception());
        } catch (...) {
        }
      }
    });
    return task_promise->get_future();
  }

  void Exec() { worker_available_cv_.notify_all(); }

  void Wait() {
    state_ = ThrdpoolState::Joining;
    std::unique_lock<std::mutex> locker(mtx_);
    task_done_cv_.wait(locker, [this] { return size_ == 0; });
    state_ = ThrdpoolState::Running;
  }

  void Stop() {
    if (state_ != ThrdpoolState::Stopped) {
      Wait();
      // stop workers and join thrds
      state_ = ThrdpoolState::Stopped;
      worker_available_cv_.notify_all();  // make thrds exit
      for (auto& thrd : thrds_) {
        thrd.join();
      }
    }
  }

  ~Thrdpool() { Stop(); };
  ThrdpoolState State() { return state_; }

 private:
  int capacity_;
  std::queue<std::function<void()>> taskpool_;
  std::atomic<ThrdpoolState> state_;
  std::atomic<size_t> size_;
  std::condition_variable worker_available_cv_;
  std::mutex mtx_;
  std::condition_variable task_done_cv_;
  std::vector<std::thread> thrds_;  // hold threads

  inline bool working() {
    return state_ == ThrdpoolState::Running || state_ == ThrdpoolState::Joining;
  }

  inline void worker() {
    while (working()) {
      std::function<void()> task;
      std::unique_lock<std::mutex> locker(mtx_);
      // only allow to weakup when has task and thrdpool is runable
      worker_available_cv_.wait(
          locker, [this] { return !taskpool_.empty() || !working(); });
      if (working()) {
        task = std::move(taskpool_.front());
        taskpool_.pop();
        locker.unlock();
        task();
        locker.lock();
        --size_;
        if (state_ == ThrdpoolState::Joining) {
          // pool is about to stop
          task_done_cv_.notify_one();
        }
      }
    }
  }
};
}  // namespace thrdpool