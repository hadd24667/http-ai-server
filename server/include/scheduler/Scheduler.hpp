#pragma once
#include <vector>

class Scheduler {
public:
    virtual ~Scheduler() {}
    virtual int selectNextTask(const std::vector<int>& tasks) = 0;

    // Factory method
    static Scheduler* createDefault();
};
