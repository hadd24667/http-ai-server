#pragma once

#include "Scheduler.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>

class RRScheduler : public Scheduler {
public:
    explicit RRScheduler(int timeSlice)
        : timeSlice_(timeSlice) {}

    std::string currentAlgorithm() const override {
        return "RR";
    }

    void setTimeSlice(int ts) override {
        timeSlice_ = ts;
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

        // Với workload HTTP hiện tại, ta chạy task một lần rồi thôi.
        // Nếu sau này bạn muốn RR "thật" với remainingTime thì chỉnh ở đây.

        return t;
    }

    bool empty() const override {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }

private:
    int timeSlice_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<Task> queue_;
};
