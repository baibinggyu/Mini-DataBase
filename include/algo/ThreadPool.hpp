#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
class ThreadPool {
public:
  ThreadPool(std::size_t size = std::thread::hardware_concurrency(),
             std::size_t queueCapacity = 1024)
      : _queueCapacity(queueCapacity) {
    if (size == 0) size = 1;
    if (queueCapacity == 0)
      throw std::invalid_argument("ThreadPool queue capacity must be positive");
    this->capacity = size;
    this->_threadVec.reserve(size);
  }
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;
  void exec() {
    std::unique_lock<std::mutex> lock(this->_mtx);
    if (this->_state != State::Pending)
      throw std::logic_error("ThreadPool can only be started once");
    try {
      for (std::size_t i = 0; i < this->capacity; i++) {
        this->_threadVec.emplace_back([this] { this->wockerLoop(); });
        ++this->_activeWorkers;
      }
      this->_state = State::Running;
    } catch (...) {
      const auto startError = std::current_exception();
      lock.unlock();
      try {
        shutdown();
      } catch (...) {
        // Preserve the thread creation error after shutdown has cleaned up.
      }
      std::rethrow_exception(startError);
    }
    lock.unlock();
    this->_conditionVariable.notify_all();
  }
  // Submissions require Running state; a full queue rejects immediately.
  template <typename F, typename... U>
  auto submit(F &&f, U &&...args)
      -> std::future<std::invoke_result_t<F, U...>> {
    using ReturnType = std::invoke_result_t<F, U...>;
    auto call = [func = std::forward<F>(f),
                 ... arguments = std::forward<U>(args)] () mutable {
      return std::invoke(std::move(func), std::move(arguments)...);
    };
    auto pacakge = std::packaged_task{std::move(call)};
    auto task = std::make_shared<decltype(pacakge)>(std::move(pacakge));
    std::future<ReturnType> result = task->get_future();
    {
      std::lock_guard<std::mutex> lock(this->_mtx);
      if (this->_state != State::Running)
        throw std::runtime_error("ThreadPool is not running");
      if (this->_taskQueue.size() >= this->_queueCapacity)
        throw std::runtime_error("ThreadPool task queue is full");
      this->_taskQueue.push([task] { (*task)(); });
    }
    this->_conditionVariable.notify_one();
    return result;
  }
  ~ThreadPool() {
    // The owner must ensure no other public method is still being called.
    this->shutdown();
  }
  void shutdown() {
    {
      std::unique_lock<std::mutex> lock(this->_mtx);
      const auto caller = std::this_thread::get_id();
      for (const std::thread &worker : this->_threadVec) {
        if (worker.get_id() == caller)
          throw std::logic_error("A ThreadPool worker cannot shut down its pool");
      }
      if (this->_state == State::Stopped)
        return;
      if (this->_state == State::Stopping) {
        this->_conditionShutdown.wait(lock, [this] {
          return this->_state == State::Stopped;
        });
        return;
      }
      this->_state = State::Stopping;
    }
    this->_conditionVariable.notify_all();
    std::exception_ptr joinError;
    for (std::thread &x : this->_threadVec) {
      if (!x.joinable()) continue;
      try {
        x.join();
      } catch (...) {
        if (!joinError) joinError = std::current_exception();
      }
    }
    if (joinError) {
      // A failed join is retried after every worker has left its loop.
      std::unique_lock<std::mutex> lock(this->_mtx);
      this->_conditionShutdown.wait(lock, [this] {
        return this->_activeWorkers == 0;
      });
      lock.unlock();
      for (std::thread &x : this->_threadVec) {
        if (!x.joinable()) continue;
        try {
          x.join();
        } catch (...) {
          // The worker has exited, so detaching cannot outlive the pool.
          try {
            x.detach();
          } catch (...) {
            std::terminate();
          }
        }
      }
    }
    {
      std::lock_guard<std::mutex> lock(this->_mtx);
      this->_state = State::Stopped;
    }
    this->_conditionShutdown.notify_all();
  }

private:
  void wockerLoop() noexcept {
    try {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(this->_mtx);
          this->_conditionVariable.wait(lock, [this]() {
            return this->_state == State::Stopped ||
                   this->_state == State::Stopping ||
                   (this->_state == State::Running &&
                    !this->_taskQueue.empty());
          });
          if (this->_state == State::Stopped ||
              (this->_state == State::Stopping && this->_taskQueue.empty()))
            break;
          if (this->_state != State::Running &&
              this->_state != State::Stopping)
            continue;
          task = std::move(this->_taskQueue.front());
          this->_taskQueue.pop();
        }
        try {
          task();
        } catch (...) {
          // packaged_task forwards user exceptions through the future;
          // this guard also keeps unexpected wrapper exceptions in the worker.
        }
      }
    } catch (...) {
      // No exception may escape a worker entry point.
    }
    {
      std::lock_guard<std::mutex> lock(this->_mtx);
      --this->_activeWorkers;
    }
    this->_conditionShutdown.notify_all();
  }

private:
  enum class State{
    Pending, Running , Stopping, Stopped
  };
  State _state{State::Pending};
  std::size_t capacity{1};
  const std::size_t _queueCapacity;
  std::size_t _activeWorkers{0};
  std::vector<std::thread> _threadVec;
  std::queue<std::function<void()>> _taskQueue;
  std::mutex _mtx;
  std::condition_variable _conditionVariable;
  std::condition_variable _conditionShutdown;
};
#endif
