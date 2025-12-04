#include "scheduler/AdaptiveScheduler.hpp"
#include "scheduler/FIFOScheduler.hpp"
#include "scheduler/SJFScheduler.hpp"
#include "scheduler/RRScheduler.hpp"
#include "scheduler/WFQScheduler.hpp"
#include "monitor/SystemMetrics.hpp"

#include <utility>

static constexpr int DEFAULT_RR_TIMESLICE = 5;

AdaptiveScheduler::AdaptiveScheduler() {
    algoName_ = "FIFO";
    inner_    = std::make_unique<FIFOScheduler>();
}

std::string AdaptiveScheduler::currentAlgorithm() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return algoName_;
}

void AdaptiveScheduler::setTimeSlice(int ts) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (auto* rr = dynamic_cast<RRScheduler*>(inner_.get())) {
        rr->setTimeSlice(ts);
    }
}

void AdaptiveScheduler::updateWeights(int /*newWeight*/) {
    // Nếu sau này bạn muốn propagate xuống WFQ thì chỉnh ở đây.
}

std::unique_ptr<Scheduler> AdaptiveScheduler::makeScheduler(const std::string& name) {
    if (name == "FIFO") {
        return std::make_unique<FIFOScheduler>();
    }
    if (name == "SJF") {
        return std::make_unique<SJFScheduler>();
    }
    if (name == "RR") {
        return std::make_unique<RRScheduler>(DEFAULT_RR_TIMESLICE);
    }
    if (name == "WFQ") {
        return std::make_unique<WFQScheduler>();
    }
    // fallback
    return std::make_unique<FIFOScheduler>();
}

void AdaptiveScheduler::maybeSwitchStrategy(double cpu, std::size_t queueLen) {
    // Rule đơn giản:
    // - Nhẹ, ít queue -> FIFO
    // - Queue dài nhưng CPU chưa quá cao -> SJF
    // - CPU cao -> RR
    // - CPU rất cao + queue dài -> WFQ
    std::string target = algoName_;

    if (queueLen < 5 && cpu < 50.0) {
        target = "FIFO";
    } else if (queueLen >= 5 && cpu < 80.0) {
        target = "SJF";
    } else if (cpu >= 80.0 && queueLen < 20) {
        target = "RR";
    } else if (cpu >= 80.0 && queueLen >= 20) {
        target = "WFQ";
    }

    if (target == algoName_) {
        return;
    }

    inner_    = makeScheduler(target);
    algoName_ = target;
}

void AdaptiveScheduler::enqueue(const Task& task) {
    enqueue(task, 0);
}

void AdaptiveScheduler::enqueue(const Task& task, std::size_t queueLen) {
    double cpu = SystemMetrics::getCpuUsage();

    std::lock_guard<std::mutex> lock(mtx_);
    maybeSwitchStrategy(cpu, queueLen);
    lastChosenAlgo_ = algoName_;

    inner_->enqueue(task);
}

Task AdaptiveScheduler::dequeue() {
    // Tránh giữ lock mtx_ trong khi chờ inner_->dequeue() (vì bên trong cũng dùng mutex/cv)
    Scheduler* innerPtr = nullptr;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        innerPtr = inner_.get();
    }

    if (!innerPtr) {
        // Trường hợp rất hiếm: inner_ null, trả về Task rỗng
        return Task{};
    }

    return innerPtr->dequeue();
}

bool AdaptiveScheduler::empty() const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!inner_) return true;
    return inner_->empty();
}
