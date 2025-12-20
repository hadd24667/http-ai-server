// ThreadPool.hpp
#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>

#include "scheduler/Task.hpp"
#include "scheduler/Scheduler.hpp"

class ThreadPool {
public:
    // ThreadPool không còn queue riêng, chỉ kéo Task từ Scheduler
    ThreadPool(int threads, Scheduler* scheduler);
    ~ThreadPool();

    // optional, để debug
    std::size_t getWorkerCount() const { return workers.size(); }

    // Không cần enqueue nữa – scheduler chịu trách nhiệm queue
    // Để không phá code cũ, bạn có thể để method rỗng:
    void enqueue(const Task& /*task*/) {
        // no-op: tất cả task phải được đưa vào scheduler, không phải ThreadPool
    }

    std::size_t getPendingTaskCount() const {
        return pendingTasks.load(std::memory_order_relaxed);
    }

    // Tăng/giảm pending task – dùng cho HttpServer
    std::size_t incrementPendingTasks() {
        return pendingTasks.fetch_add(1, std::memory_order_relaxed);
    }

    void decrementPendingTasks() {
        pendingTasks.fetch_sub(1, std::memory_order_relaxed);
    }

    void notifyWorker() {
        cv.notify_one();
    }

private:
    void workerLoop();

    Scheduler* scheduler;
    std::vector<std::thread> workers;
    std::atomic<bool> stop{false};

    // Đếm task đang "trong hệ thống" (đang chờ + đang chạy)
    std::atomic<std::size_t> pendingTasks{0};

    std::condition_variable cv;
    std::mutex queueMutex;
};
