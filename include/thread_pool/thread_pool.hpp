// Copyright (c) 2020 Robert Vaser

#ifndef THREAD_POOL_THREAD_POOL_HPP_
#define THREAD_POOL_THREAD_POOL_HPP_

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <future>  // NOLINT
#include <memory>
#include <queue>
#include <string>
#include <thread>  // NOLINT
#include <unordered_map>
#include <utility>
#include <vector>

#include "thread_pool/semaphore.hpp"

namespace thread_pool {

class ThreadPool {
 public:
  ThreadPool(std::uint32_t num_threads = std::thread::hardware_concurrency() / 2)  // NOLINT
      : threads_(),
        thread_ids_(),
        thread_semaphore_(0),
        queue_(),
        queue_semaphore_(1),
        terminate_(false) {
    num_threads = std::max(1U, num_threads);
    while (num_threads-- != 0) {
      threads_.emplace_back(ThreadPool::Task, this);
      thread_ids_.emplace(threads_.back().get_id(), threads_.size() - 1);
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  ~ThreadPool() {
    terminate_ = true;
    for (std::uint32_t i = 0; i < threads_.size(); ++i) {
      thread_semaphore_.Signal();
    }
    for (auto& it : threads_) {
      it.join();
    }
  }

  std::uint32_t num_threads() const {
    return threads_.size();
  }

  const std::unordered_map<std::thread::id, std::uint32_t>& thread_ids() const {
    return thread_ids_;
  }

  template<typename T, typename... Ts>
  auto Submit(T&& routine, Ts&&... params)
      -> std::future<typename std::result_of<T(Ts...)>::type> {
    auto task = std::make_shared<std::packaged_task<typename std::result_of<T(Ts...)>::type()>>(  // NOLINT
        std::bind(std::forward<T>(routine), std::forward<Ts>(params)...));
    auto task_result = task->get_future();
    auto task_wrapper = [task] () {
      (*task)();
    };

    queue_semaphore_.Wait();
    queue_.emplace(task_wrapper);
    queue_semaphore_.Signal();

    thread_semaphore_.Signal();
    return task_result;
  }

 private:
  static void Task(ThreadPool* thread_pool) {
    while (true) {
      thread_pool->thread_semaphore_.Wait();

      if (thread_pool->terminate_) {
        break;
      }

      thread_pool->queue_semaphore_.Wait();
      auto task = std::move(thread_pool->queue_.front());
      thread_pool->queue_.pop();
      thread_pool->queue_semaphore_.Signal();

      if (thread_pool->terminate_) {
        break;
      }

      task();
    }
  }

  std::vector<std::thread> threads_;
  std::unordered_map<std::thread::id, std::uint32_t> thread_ids_;
  Semaphore thread_semaphore_;
  std::queue<std::function<void()>> queue_;
  Semaphore queue_semaphore_;
  std::atomic<bool> terminate_;
};

}  // namespace thread_pool

#endif  // THREAD_POOL_THREAD_POOL_HPP_
