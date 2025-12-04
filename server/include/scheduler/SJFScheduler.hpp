#pragma once

#include "Scheduler.hpp"
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>

class SJFScheduler : public Scheduler {
public:
    SJFScheduler() = default;

    std::string currentAlgorithm() const override {
        return "SJF";
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
        // Ưu tiên estimatedTime nhỏ hơn trước
        bool operator()(const Task& a, const Task& b) const {
            return a.estimatedTime > b.estimatedTime;
        }
    };

    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::priority_queue<Task, std::vector<Task>, Compare> pq_;
};
