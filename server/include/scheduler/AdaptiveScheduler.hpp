#pragma once

#include "scheduler/Scheduler.hpp"
#include <memory>
#include <mutex>
#include <string>

class AdaptiveScheduler : public Scheduler {
public:
    AdaptiveScheduler();

    // API kế thừa từ Scheduler
    std::string currentAlgorithm() const override;

    void enqueue(const Task& task) override;
    void enqueue(const Task& task, std::size_t queueLen) override;

    Task dequeue() override;

    bool empty() const override;

    void setTimeSlice(int ts) override;
    void updateWeights(int newWeight) override;

private:
    // Scheduler đang active (FIFO/SJF/RR/WFQ)
    std::unique_ptr<Scheduler> inner_;

    // tên thuật toán hiện tại (được chọn sau khi switch)
    std::string algoName_;

    // thuật toán được chọn ngay thời điểm enqueue (log thôi)
    std::string lastChosenAlgo_;

    // đồng bộ hoá
    mutable std::mutex mtx_;

    // rule-based switching
    void maybeSwitchStrategy(double cpu, std::size_t queueLen);

    // factory tạo scheduler mới khi cần switch
    std::unique_ptr<Scheduler> makeScheduler(const std::string& name);
};
