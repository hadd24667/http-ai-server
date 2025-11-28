#include "threadpool/ThreadPool.hpp"
#include "scheduler/Task.hpp"
#include <iostream>

ThreadPool::ThreadPool(int threads) : stop(false) {
    for (int i = 0; i < threads; i++) {
        workers.emplace_back([this]() {
            while (true) {
                Task t;

                {
                    std::unique_lock<std::mutex> lock(mtx);
                    cv.wait(lock, [this]() {
                        return stop || !tasks.empty();
                    });

                    if (stop && tasks.empty()) return;

                    t = tasks.top();
                    tasks.pop();
                }

                t.func();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        stop = true;
    }
    cv.notify_all();

    for (auto &w : workers)
        if (w.joinable()) w.join();
}

void ThreadPool::enqueue(const Task& task) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push(task);
    }
    cv.notify_one();
}

std::size_t ThreadPool::getPendingTaskCount() const {
    std::lock_guard<std::mutex> lock(mtx);
    return tasks.size();
}
