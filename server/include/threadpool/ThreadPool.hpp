#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "scheduler/Task.hpp"

struct TaskComparator {
    bool operator()(const Task& a, const Task& b) const {
        return a.estimatedTime > b.estimatedTime; // SJF
    }
};

class ThreadPool {
public:
    ThreadPool(int threads);
    ~ThreadPool();

    void enqueue(const Task& task);
    std::size_t getPendingTaskCount() const;

private:
    mutable std::mutex mtx;
    std::condition_variable cv;
    bool stop;

    std::priority_queue<Task, std::vector<Task>, TaskComparator> tasks;
    std::vector<std::thread> workers;
};
