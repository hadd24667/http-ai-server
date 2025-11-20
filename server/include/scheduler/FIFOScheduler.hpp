#pragma once
#include "Scheduler.hpp"
#include <iostream>
#include <queue>

class FIFOScheduler : public Scheduler {
private:
    std::queue<Task> q;

public:
   void enqueue(const Task& task) override {
    std::cout << "[SCHED] enqueue FIFO task id=" << task.id << "\n";
    q.push(task);
}

    Task dequeue() override {
        Task t = q.front();
        q.pop();
        std::cout << "[SCHED] dequeue FIFO -> task id=" << t.id << "\n";
        return t;
    }

};
