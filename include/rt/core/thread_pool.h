#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace rt {

class TaskGroup;

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency())
        : stop_(false) {
        if (numThreads == 0) numThreads = 1;
        workers_.reserve(numThreads);
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex_);
                        cvWorker_.wait(lock, [this]() {
                            return stop_ || !tasks_.empty();
                        });
                        if (stop_ && tasks_.empty()) {
                            return;
                        }
                        if (!tasks_.empty()) {
                            task = std::move(tasks_.front());
                            tasks_.pop_front();
                        }
                    }
                    if (task) {
                        task();
                    }
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }
        cvWorker_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    size_t ThreadCount() const {
        return workers_.size();
    }

    void Enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            tasks_.push_back(std::move(task));
        }
        cvWorker_.notify_one();
    }

    bool TryExecuteOne() {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (tasks_.empty()) {
                return false;
            }
            task = std::move(tasks_.front());
            tasks_.pop_front();
        }
        if (task) {
            task();
            return true;
        }
        return false;
    }

    static ThreadPool& Default() {
        static ThreadPool defaultPool;
        return defaultPool;
    }

private:
    std::vector<std::thread> workers_;
    std::deque<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable cvWorker_;
    bool stop_;
};

class TaskGroup {
public:
    explicit TaskGroup(ThreadPool& pool = ThreadPool::Default())
        : pool_(pool), outstandingTasks_(0) {}

    ~TaskGroup() {
        Wait();
    }

    void Run(std::function<void()> task) {
        outstandingTasks_.fetch_add(1, std::memory_order_release);
        pool_.Enqueue([this, task = std::move(task)]() {
            task();
            if (outstandingTasks_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                std::unique_lock<std::mutex> lock(mutex_);
                cvDone_.notify_all();
            }
        });
    }

    void Wait() {
        while (outstandingTasks_.load(std::memory_order_acquire) > 0) {
            if (!pool_.TryExecuteOne()) {
                std::unique_lock<std::mutex> lock(mutex_);
                if (outstandingTasks_.load(std::memory_order_acquire) > 0) {
                    cvDone_.wait_for(lock, std::chrono::microseconds(20), [this]() {
                        return outstandingTasks_.load(std::memory_order_acquire) == 0;
                    });
                }
            }
        }
    }

    ThreadPool& Pool() const { return pool_; }

private:
    ThreadPool& pool_;
    std::atomic<int> outstandingTasks_;
    std::mutex mutex_;
    std::condition_variable cvDone_;
};

}
