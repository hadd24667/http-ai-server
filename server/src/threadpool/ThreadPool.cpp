#include "threadpool/ThreadPool.hpp"
#include <iostream>

ThreadPool::ThreadPool(int threads) : stop(false) {
    for (int i = 0; i < threads; i++) {
        workers.emplace_back([this]() {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(mutexQueue);

                    cv.wait(lock, [this]() {
                        return stop || !tasks.empty();
                    });

                    if (stop && tasks.empty()) return;

                    task = std::move(tasks.front());
                    tasks.pop();
                }

                std::cout << "[DEBUG] Worker executing task\n";
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    stop = true;
    cv.notify_all();

    for (auto &worker : workers)
        if (worker.joinable()) worker.join();
}

void ThreadPool::enqueue(std::function<void()> task) {
    std::cout << "[DEBUG] Enqueuing task\n";
    {
        std::lock_guard<std::mutex> lock(mutexQueue);
        tasks.push(std::move(task));
    }
    cv.notify_one();
}
