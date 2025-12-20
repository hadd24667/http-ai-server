#include "scheduler/AdaptiveScheduler.hpp"
#include "scheduler/FIFOScheduler.hpp"
#include "scheduler/SJFScheduler.hpp"
#include "scheduler/RRScheduler.hpp"
#include "scheduler/WFQScheduler.hpp"
#include "monitor/SystemMetrics.hpp"

#include <numeric>
#include <algorithm>

// ================================
//  THAM SỐ ADAPTIVE MỚI
// ================================
static constexpr int RR_TIMESLICE_DEFAULT = 5;

// Cửa sổ trượt để đo biến thiên workload (path length)
static constexpr int WORKLOAD_WINDOW = 40;


// ================================
//  Constructor
// ================================
AdaptiveScheduler::AdaptiveScheduler() {
    algoName_ = "FIFO";
    inner_    = std::make_unique<FIFOScheduler>();
}


// ================================
//  current algo
// ================================
std::string AdaptiveScheduler::currentAlgorithm() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return algoName_;
}


// ================================
//  Helper: Tính độ biến thiên workload
// ================================
double AdaptiveScheduler::workloadVariability() {
    std::lock_guard<std::mutex> lock(wloadMtx_);

    if (recentWorkloads_.size() < 5) return 0.0;

    double avg = std::accumulate(recentWorkloads_.begin(),
                                 recentWorkloads_.end(), 0.0)
                 / recentWorkloads_.size();

    double var = 0.0;
    for (double w : recentWorkloads_) {
        var += (w - avg) * (w - avg);
    }
    var /= recentWorkloads_.size();
    return var;    // variance
}


// ================================
//  Adaptive decision
// ================================
std::string AdaptiveScheduler::decideAlgorithm(double cpu,
                                               std::size_t qlen,
                                               double wvar)
{
    // 1) Load rất nhỏ → FIFO
    if (qlen < 20 && cpu < 40.0) {
        return "FIFO";
    }

    // 2) Workload ít biến thiên + CPU chưa quá cao → SJF
    if (wvar < 200.0 && cpu < 70.0) {
        return "SJF";
    }

    // 3) CPU cao → RR (queue chưa phình to)
    if (cpu >= 70.0 && cpu < 85.0 && qlen < 200) {
        return "RR";
    }

    // 4) CPU rất cao + queue phình lớn → WFQ
    if (cpu >= 85.0 || qlen >= 200) {
        return "WFQ";
    }

    // fallback tự nhiên
    return "SJF";
}


// ================================
//  Scheduler factory
// ================================
std::unique_ptr<Scheduler> AdaptiveScheduler::make(const std::string& name) {
    if (name == "FIFO") return std::make_unique<FIFOScheduler>();
    if (name == "SJF")  return std::make_unique<SJFScheduler>();
    if (name == "RR")   return std::make_unique<RRScheduler>(RR_TIMESLICE_DEFAULT);
    if (name == "WFQ")  return std::make_unique<WFQScheduler>();
    return std::make_unique<FIFOScheduler>();
}


// ================================
//  enqueue(x): nơi quyết định thuật toán
// ================================
void AdaptiveScheduler::enqueue(const Task& t) {
    enqueue(t, 0);
}

void AdaptiveScheduler::enqueue(const Task& t, std::size_t queueLen) {
    double cpu = SystemMetrics::getCpuUsage();

    // record workload để đo biến thiên
    {
        std::lock_guard<std::mutex> lk(wloadMtx_);
        recentWorkloads_.push_back(t.estimatedTime);
        if (recentWorkloads_.size() > WORKLOAD_WINDOW)
            recentWorkloads_.erase(recentWorkloads_.begin());
    }

    // lấy variance
    double wvar = workloadVariability();

    // quyết định thuật toán
    std::string target = decideAlgorithm(cpu, queueLen, wvar);

    std::lock_guard<std::mutex> lock(mtx_);

    // đổi inner scheduler nếu cần
    if (target != algoName_) {
        inner_    = make(target);
        algoName_ = target;
    }

    // enqueue vào scheduler hiện tại
    inner_->enqueue(t);
}


// ================================
//  dequeue
// ================================
Task AdaptiveScheduler::dequeue() {
    Scheduler* ptr = nullptr;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        ptr = inner_.get();
    }
    if (!ptr) return Task{};
    return ptr->dequeue();
}


// ================================
//  empty()
// ================================
bool AdaptiveScheduler::empty() const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!inner_) return true;
    return inner_->empty();
}
