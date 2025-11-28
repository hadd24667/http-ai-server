#pragma once
#include "scheduler/Scheduler.hpp"
#include <memory>
#include <string>
#include <mutex>

class AdaptiveScheduler : public Scheduler {
public:
    AdaptiveScheduler();

    // API kế thừa từ Scheduler
    std::string currentAlgorithm() const override;

    void enqueue(const Task& task) override;
    void enqueue(const Task& task, std::size_t queueLen) override;

    Task dequeue() override;

private:
    // Scheduler đang active (FIFO/SJF/RR/WFQ)
    std::unique_ptr<Scheduler> inner;

    // tên thuật toán hiện tại (được chọn sau khi switch)
    std::string algoName;

    // thuật toán được chọn ngay thời điểm enqueue
    std::string lastChosenAlgo;

    // đồng bộ hoá
    mutable std::mutex mtx;

    // rule-based switching
    void maybeSwitchStrategy(double cpu, std::size_t queueLen);

    // factory tạo scheduler mới khi cần switch
    std::unique_ptr<Scheduler> makeScheduler(const std::string& name);
};
