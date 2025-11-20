#pragma once
#include "Scheduler.hpp"
#include <iostream>
#include <queue>

class SJFScheduler : public Scheduler {
private:
    struct Compare {
        bool operator()(const Task& a, const Task& b) const {
            return a.estimatedTime > b.estimatedTime;
        }
    };

    std::priority_queue<Task, std::vector<Task>, Compare> pq;

public:
    void enqueue(const Task& task) override {
        std::cout << "[SCHED] enqueue SJF id=" << task.id 
          << " est=" << task.estimatedTime << "\n";

        pq.push(task);
    }

    Task dequeue() override {
        Task t = pq.top();
        pq.pop();
        std::cout << "[SCHED] dequeue SJF -> task id=" << t.id << "\n";
        return t;
    }
};
