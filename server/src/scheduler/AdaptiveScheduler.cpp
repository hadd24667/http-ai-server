#include "scheduler/AdaptiveScheduler.hpp"
#include "scheduler/FifoScheduler.hpp"
#include "scheduler/SjfScheduler.hpp"
#include "scheduler/RrScheduler.hpp"
#include "scheduler/WfqScheduler.hpp"
#include "monitor/SystemMetrics.hpp"

// TODO: tuỳ bạn define RR timeslice
static constexpr int RR_TIMESLICE = 5;

AdaptiveScheduler::AdaptiveScheduler()
{
    algoName = "FIFO";
    inner = std::make_unique<FifoScheduler>();
}

std::unique_ptr<Scheduler> AdaptiveScheduler::makeScheduler(const std::string& name) {
    if (name == "FIFO") {
        return std::make_unique<FifoScheduler>();
    } else if (name == "SJF") {
        return std::make_unique<SjfScheduler>();
    } else if (name == "RR") {
        return std::make_unique<RrScheduler>(RR_TIMESLICE);
    } else if (name == "WFQ") {
        return std::make_unique<WfqScheduler>();
    }
    // fallback
    return std::make_unique<FifoScheduler>();
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
    // Đo CPU + queueLen hiện tại để quyết định
    double cpu = SystemMetrics::getCpuUsage();
    // queueLen: tạm thời dùng workload (hoặc hook từ ngoài vào nếu cần)
    // ở đây AdaptiveScheduler tự không biết queueLen của ThreadPool,
    // nên bạn có thể inject queueLen từ HttpServer thay vì đo ở đây.
    std::size_t queueLen = 0; // sẽ set từ ngoài nếu muốn

    {
        std::lock_guard<std::mutex> lock(mtx);
        maybeSwitchStrategy(cpu, queueLen);
        inner->enqueue(task);
    }
}

Task AdaptiveScheduler::dequeue() {
    std::lock_guard<std::mutex> lock(mtx);
    return inner->dequeue();
}
