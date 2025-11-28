#include "scheduler/AdaptiveScheduler.hpp"
#include "scheduler/FIFOScheduler.hpp"
#include "scheduler/SJFScheduler.hpp"
#include "scheduler/RRScheduler.hpp"
#include "scheduler/WFQScheduler.hpp"
#include "monitor/SystemMetrics.hpp"

// TODO: tuỳ bạn define RR timeslice
static constexpr int RR_TIMESLICE = 5;

AdaptiveScheduler::AdaptiveScheduler()
{
    algoName = "FIFO";
    inner = std::make_unique<FIFOScheduler>();
}

std::unique_ptr<Scheduler> AdaptiveScheduler::makeScheduler(const std::string& name) {
    if (name == "FIFO") {
        return std::make_unique<FIFOScheduler>();
    } else if (name == "SJF") {
        return std::make_unique<SJFScheduler>();
    } else if (name == "RR") {
        return std::make_unique<RRScheduler>(RR_TIMESLICE);
    } else if (name == "WFQ") {
        return std::make_unique<WFQScheduler>();
    }
    // fallback
    return std::make_unique<FIFOScheduler>();
}

std::string AdaptiveScheduler::currentAlgorithm() const {
    std::lock_guard<std::mutex> lock(mtx);
    return algoName;
}

void AdaptiveScheduler::maybeSwitchStrategy(double cpu, std::size_t queueLen) {
    // rule đơn giản, bạn có thể chỉnh mạnh hơn
    std::string next = algoName;

    if (queueLen < 5) {
        next = "FIFO";
    } else if (queueLen >= 5 && queueLen < 20 && cpu < 50.0) {
        next = "SJF";
    } else if (cpu >= 50.0 && cpu < 80.0) {
        next = "RR";
    } else if (cpu >= 80.0) {
        next = "WFQ";
    }

    if (next != algoName) {
        // đơn giản: bỏ queue cũ, tạo scheduler mới
        inner = makeScheduler(next);
        algoName = next;
    }
}

void AdaptiveScheduler::enqueue(const Task& task) {
    enqueue(task, 0);
}

void AdaptiveScheduler::enqueue(const Task& task, std::size_t qlen) {
    double cpu = SystemMetrics::getCpuUsage();

    std::lock_guard<std::mutex> lock(mtx);
    maybeSwitchStrategy(cpu, qlen);
    lastChosenAlgo = algoName;   // thuật toán được chọn tại ENQUEUE


    inner->enqueue(task);
}   

Task AdaptiveScheduler::dequeue() {
    std::lock_guard<std::mutex> lock(mtx);
    return inner->dequeue();
}
