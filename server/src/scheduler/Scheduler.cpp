#include "scheduler/Scheduler.hpp"

class FifoScheduler : public Scheduler {
public:
    int selectNextTask(const std::vector<int>& tasks) override {
        if (tasks.empty()) return -1;
        return 0; // luôn chọn task đầu (FIFO)
    }
};

Scheduler* Scheduler::createDefault() {
    return new FifoScheduler();
}
