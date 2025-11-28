#pragma once
#include "Scheduler.hpp"
#include <queue>

class FIFOScheduler : public Scheduler {
public:
    std::string currentAlgorithm() const override { return "FIFO"; }

    void enqueue(const Task& task, std::size_t queueLen) override {
        queue.push(task);
    }

    void enqueue(const Task& task) override {
        queue.push(task);
    }

    Task dequeue() override {
        Task t = queue.front();
        queue.pop();
        return t;
    }

private:
    std::queue<Task> queue;
};
