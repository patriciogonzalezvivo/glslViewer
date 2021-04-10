// Copyright (c) 2020 Robert Vaser

#ifndef THREAD_POOL_SEMAPHORE_HPP_
#define THREAD_POOL_SEMAPHORE_HPP_

#include <condition_variable>  // NOLINT
#include <cstdint>
#include <memory>
#include <mutex>  // NOLINT

namespace thread_pool {

class Semaphore {
 public:
  explicit Semaphore(std::uint32_t count)
      : count_(count) {}

  Semaphore(const Semaphore&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;

  Semaphore(Semaphore&&) = delete;
  Semaphore& operator=(Semaphore&&) = delete;

  ~Semaphore() = default;

  void Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [&] () { return count_; });
    --count_;
  }

  void Signal() {
    std::unique_lock<std::mutex> lock(mutex_);
    ++count_;
    condition_.notify_one();
  }

 private:
  std::uint32_t count_;
  std::mutex mutex_;
  std::condition_variable condition_;
};

}  // namespace thread_pool

#endif  // THREAD_POOL_SEMAPHORE_HPP_
