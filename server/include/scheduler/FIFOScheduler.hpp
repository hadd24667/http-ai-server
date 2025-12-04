#pragma once

#include "Scheduler.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>

class FIFOScheduler : public Scheduler {
public:
    FIFOScheduler() = default;

    std::string currentAlgorithm() const override {
        return "FIFO";
    }

    void enqueue(const Task& task) override {
        enqueue(task, 0);
    }

    void enqueue(const Task& task, std::size_t /*queueLen*/) override {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            queue_.push(task);
        }
        cv_.notify_one();
    }

    Task dequeue() override {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this]() {
            return !queue_.empty();
        });

        Task t = queue_.front();
        queue_.pop();
        return t;
    }

    bool empty() const override {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }

private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<Task> queue_;
};
