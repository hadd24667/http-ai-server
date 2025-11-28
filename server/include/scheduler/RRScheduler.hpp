#pragma once
#include "Scheduler.hpp"
#include <queue>

class RRScheduler : public Scheduler {
public:
    explicit RRScheduler(int ts) : timeslice(ts) {}

    std::string currentAlgorithm() const override { return "RR"; }

    void setTimeSlice(int ts) override { timeslice = ts; }

    void enqueue(const Task& task, std::size_t queueLen) override {
        queue.push(task);
    }

    void enqueue(const Task& task) override {
        queue.push(task);
    }

    Task dequeue() override {
        Task t = queue.front();
        queue.pop();

        // giảm remainingTime
        t.remainingTime -= timeslice;
        if (t.remainingTime > 0) {
            queue.push(t);
        }

        return t;
    }

private:
    int timeslice;
    std::queue<Task> queue;
};
