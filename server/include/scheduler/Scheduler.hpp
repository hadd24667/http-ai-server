#pragma once
#include "Task.hpp"
#include <memory>

class Scheduler {
public:
    virtual ~Scheduler() = default;

    // Add new task to scheduler queue
    virtual void enqueue(const Task& task) = 0;

    // Pop next task based on scheduling policy
    virtual Task dequeue() = 0;

    // Optional: for RR (you can override if needed)
    virtual void setTimeSlice(int ts) {}

    // Optional: for WFQ
    virtual void updateWeights(int newWeight) {}

};
