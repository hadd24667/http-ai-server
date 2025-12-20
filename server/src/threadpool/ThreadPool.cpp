#include "threadpool/ThreadPool.hpp"
#include <iostream>
#include <thread>
#include <chrono>

static inline long long nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}

#define LOGT(msg) \
    std::cout << "[" << nowMs() << "ms]" \
              << "[TID " << std::this_thread::get_id() << "] " \
              << msg << std::endl;


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
            cv.wait(lock, [this]() {
                bool ready = stop.load(std::memory_order_relaxed) || !scheduler->empty();
                return ready;
            });
            if (stop.load(std::memory_order_relaxed)) {
                return;
            }
            t = scheduler->dequeue();
        }

        if (t.fn) {
            t.fn();
        } else {
            LOGT("⚠️ Got empty task (fn=null)");
        }
        pendingTasks.fetch_sub(1, std::memory_order_relaxed);
    }
}




