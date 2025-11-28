#pragma once
#include <atomic>

class SystemMetrics {
public:
    static void init();
    static double getCpuUsage();

private:
    static std::atomic<double> CPU_CACHE;
};
