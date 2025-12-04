#include "threadpool/ThreadPool.hpp"
#include <iostream>
#include <chrono>

ThreadPool::ThreadPool(int threads, Scheduler* scheduler)
    : scheduler(scheduler), stop(false)
{
    if (!scheduler) {
        throw std::runtime_error("ThreadPool requires a non-null Scheduler*");
    }

    for (int i = 0; i < threads; ++i) {
        workers.emplace_back([this]() {
            workerLoop();
        });
    }
}

ThreadPool::~ThreadPool() {
    stop.store(true, std::memory_order_relaxed);

    // Ở thiết kế hiện tại, scheduler->dequeue() có thể block hoặc không.
    // Nếu không block và trả Task “rỗng” khi hết việc, bạn có thể break loop phía dưới.
    for (auto& w : workers) {
        if (w.joinable()) {
            w.join();
        }
    }
}

void ThreadPool::workerLoop() {
    while (true) {
        Task t;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            // Chờ đến khi có task thật sự trong ThreadPool
            cv.wait(lock, [this]() {
                return stop.load(std::memory_order_relaxed) ||
                    !scheduler->empty();
            });
            if (stop.load(std::memory_order_relaxed))
                return;

            // lấy task từ scheduler
            t = scheduler->dequeue();
        }

        // chạy task
        if (t.func) {
            t.func();
        }

        // giảm task pending
        pendingTasks.fetch_sub(1, std::memory_order_relaxed);
    }
}



