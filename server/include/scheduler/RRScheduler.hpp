#pragma once
#include "Scheduler.hpp"
#include <iostream>
#include <queue>

class RRScheduler : public Scheduler {
private:
    std::queue<Task> q;
    int timeSlice;

public:
     RRScheduler(int ts) : timeSlice(ts) {}

    void setTimeSlice(int ts) override {
        timeSlice = ts;
    }

    void enqueue(const Task& task) override {
        std::cout << "[SCHED] RR dequeue id=" << task.id 
          << " remain=" << task.remainingTime << "\n";
        q.push(task);
    }

    Task dequeue() override {
        Task t = q.front();
        q.pop();

        // Giảm thời gian còn lại
        int exec = std::min(timeSlice, t.remainingTime);
        t.remainingTime -= exec;

        if (t.remainingTime > 0)
            q.push(t);  // tái xếp cuối hàng

        std::cout << "[SCHED] dequeue RR -> task id=" << t.id << "\n";
        return t;
    }
};
