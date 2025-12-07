#pragma once

#include "Scheduler.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>

class RRScheduler : public Scheduler {
public:
    explicit RRScheduler(int timeSlice = 5)
        : timeSlice_(timeSlice) {}

    std::string currentAlgorithm() const override {
        return "RR";
    }

    void setTimeSlice(int ts) override {
        timeSlice_ = ts;
    }

    void enqueue(const Task& task) override {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(task);
        cv_.notify_one();
    }

    Task dequeue() override {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] {
            return !queue_.empty();
        });

        Task t = queue_.front();
        queue_.pop();

        // ============================
        // ROUND ROBIN CORE LOGIC
        // ============================
        if (t.remainingTime > timeSlice_) {
            t.remainingTime -= timeSlice_;
            queue_.push(t);               // push back as unfinished
        } else {
            t.remainingTime = 0;          // finished
        }

        return t;
    }

    bool empty() const override {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }

private:
    int timeSlice_;
    std::queue<Task> queue_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
};
