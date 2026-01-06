#pragma once

#include "Scheduler.hpp"
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

class WFQScheduler : public Scheduler {
public:
    WFQScheduler() : virtualTime_(0.0) {}

    std::string currentAlgorithm() const override {
        return "WFQ";
    }

    void enqueue(const Task& task) override {
        std::lock_guard<std::mutex> lock(mtx_);

        double nowV = virtualTime_;

        double lastFinish = lastFinishTime_[task.weight]; // nhóm theo weight (flow)
        double S = std::max(lastFinish, nowV);
        double F = S + (double)task.estimatedTime / std::max(1, task.weight);

        // Lưu lại finish time flow
        lastFinishTime_[task.weight] = F;

        // đẩy vào PQ
        pq_.push(WFQItem{task, S, F});
        cv_.notify_one();
    }

    Task dequeue() override {
        std::unique_lock<std::mutex> lock(mtx_);

        cv_.wait(lock, [this] { return !pq_.empty(); });

        WFQItem item = pq_.top();
        pq_.pop();

        // tăng virtual time
        virtualTime_ = item.finishTime;

        return item.task;
    }

    bool empty() const override {
        std::lock_guard<std::mutex> lock(mtx_);
        return pq_.empty();
    }

private:
    struct WFQItem {
        Task task;
        double startTime;
        double finishTime;

        bool operator<(WFQItem const& other) const {
            return finishTime > other.finishTime; 
        }
    };

    double virtualTime_;
    std::unordered_map<int, double> lastFinishTime_;

    std::priority_queue<WFQItem> pq_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
};
