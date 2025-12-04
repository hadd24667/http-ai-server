#pragma once

#include "Scheduler.hpp"
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>

class WFQScheduler : public Scheduler {
public:
    WFQScheduler() = default;

    std::string currentAlgorithm() const override {
        return "WFQ";
    }

    void enqueue(const Task& task) override {
        enqueue(task, 0);
    }

    void enqueue(const Task& task, std::size_t /*queueLen*/) override {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            pq_.push(task);
        }
        cv_.notify_one();
    }

    Task dequeue() override {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this]() {
            return !pq_.empty();
        });

        Task t = pq_.top();
        pq_.pop();
        return t;
    }

    bool empty() const override {
        std::lock_guard<std::mutex> lock(mtx_);
        return pq_.empty();
    }

private:
    struct Compare {
        // Đơn giản hoá: ưu tiên "estimatedTime / weight" nhỏ hơn trước
        bool operator()(const Task& a, const Task& b) const {
            double wa = (a.weight > 0) ? static_cast<double>(a.weight) : 1.0;
            double wb = (b.weight > 0) ? static_cast<double>(b.weight) : 1.0;
            double fa = static_cast<double>(a.estimatedTime) / wa;
            double fb = static_cast<double>(b.estimatedTime) / wb;
            return fa > fb; // min-heap trên finish time
        }
    };

    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::priority_queue<Task, std::vector<Task>, Compare> pq_;
};
