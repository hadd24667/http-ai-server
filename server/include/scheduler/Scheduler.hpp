#pragma once
#include "Task.hpp"
#include <memory>
#include <string>

class Scheduler {
public:
    virtual ~Scheduler() = default;

    // Add new task to scheduler queue
    virtual void enqueue(const Task& task) = 0;

    virtual void enqueue(const Task& task, std::size_t queueLen) {
    // default: gọi hàm cũ (để không phá các scheduler khác)
    enqueue(task);
    }

    // Pop next task based on scheduling policy
    virtual Task dequeue() = 0;

    // Optional: for RR (you can override if needed)
    virtual void setTimeSlice(int ts) {}

    // Optional: for WFQ
    virtual void updateWeights(int newWeight) {}

    virtual std::string currentAlgorithm() const = 0;

};
