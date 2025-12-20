#pragma once

#include "scheduler/Scheduler.hpp"
#include "ai/AIClient.hpp"
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class AdaptiveScheduler : public Scheduler {
public:
    AdaptiveScheduler();

    // Tên thuật toán hiện tại
    std::string currentAlgorithm() const override;

    // enqueue
    void enqueue(const Task& task) override;
    void enqueue(const Task& task, std::size_t queueLen) override;

    // dequeue
    Task dequeue() override;

    bool empty() const override;

    // (Không dùng trong adaptive mới, nhưng giữ để tránh lỗi interface)
    void setTimeSlice(int ts) override {}
    void updateWeights(int newWeight) override {}

private:
    // -------------------------------
    //   Scheduler bên trong (FIFO/SJF/RR/WFQ)
    // -------------------------------
    std::unique_ptr<Scheduler> inner_;
    std::string algoName_;

    // Mutex bảo vệ state
    mutable std::mutex mtx_;

    // -------------------------------
    //   Adaptive workload tracking
    // -------------------------------
    std::vector<int> recentWorkloads_;
    mutable std::mutex wloadMtx_;

    // Tính biến thiên workload (variance)
    double workloadVariability();

    // Thuật toán quyết định thuật toán lập lịch
    std::string decideAlgorithm(double cpu, std::size_t queueLen, double wvar);

    // Factory tạo scheduler
    std::unique_ptr<Scheduler> make(const std::string& name);

    bool aiEnabled_ = true;
    std::unique_ptr<AIClient> ai_;
    std::chrono::steady_clock::time_point lastAiCall_{};
    std::chrono::milliseconds aiMinInterval_{200};
};
