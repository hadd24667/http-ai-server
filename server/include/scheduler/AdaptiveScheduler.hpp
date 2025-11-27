#pragma once
#include "scheduler/Scheduler.hpp"
#include <memory>
#include <string>
#include <mutex>

class AdaptiveScheduler : public Scheduler {
public:
    AdaptiveScheduler();

    void enqueue(const Task& task) override;
    Task dequeue() override;

    std::string currentAlgorithm() const override;

private:
    std::unique_ptr<Scheduler> inner;
    std::string algoName;
    mutable std::mutex mtx;

    void maybeSwitchStrategy(double cpu, std::size_t queueLen);
    std::unique_ptr<Scheduler> makeScheduler(const std::string& name);
};
