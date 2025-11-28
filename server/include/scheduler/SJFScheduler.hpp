#pragma once
#include "Scheduler.hpp"
#include <queue>
#include <vector>

class SJFScheduler : public Scheduler {
public:
    std::string currentAlgorithm() const override { return "SJF"; }

    struct Cmp {
        bool operator()(const Task& a, const Task& b) const {
            return a.estimatedTime > b.estimatedTime;
        }
    };

    void enqueue(const Task& task, std::size_t queueLen) override {
        pq.push(task);
    }

    void enqueue(const Task& task) override {
        pq.push(task);
    }

    Task dequeue() override {
        Task t = pq.top();
        pq.pop();
        return t;
    }

private:
    std::priority_queue<Task, std::vector<Task>, Cmp> pq;
};
