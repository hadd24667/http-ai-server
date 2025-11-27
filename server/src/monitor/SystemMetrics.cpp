#include "monitor/SystemMetrics.hpp"
#include <fstream>
#include <thread>
#include <chrono>

namespace {
    struct CpuTimes {
        uint64_t user = 0, nice = 0, system = 0, idle = 0;
        uint64_t iowait = 0, irq = 0, softirq = 0, steal = 0;
    };

    bool readCpuTimes(CpuTimes& t) {
        std::ifstream f("/proc/stat");
        if (!f.is_open()) return false;

        std::string cpu;
        f >> cpu;
        if (cpu != "cpu") return false;

        f >> t.user >> t.nice >> t.system >> t.idle
          >> t.iowait >> t.irq >> t.softirq >> t.steal;

        return true;
    }
}

double SystemMetrics::getCpuUsage() {
    CpuTimes t1{}, t2{};
    if (!readCpuTimes(t1)) return 0.0;

    // ngủ 100ms để lấy delta
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (!readCpuTimes(t2)) return 0.0;

    uint64_t idle1 = t1.idle + t1.iowait;
    uint64_t idle2 = t2.idle + t2.iowait;

    uint64_t nonIdle1 = t1.user + t1.nice + t1.system + t1.irq + t1.softirq + t1.steal;
    uint64_t nonIdle2 = t2.user + t2.nice + t2.system + t2.irq + t2.softirq + t2.steal;

    uint64_t total1 = idle1 + nonIdle1;
    uint64_t total2 = idle2 + nonIdle2;

    double totalDelta = static_cast<double>(total2 - total1);
    double idleDelta  = static_cast<double>(idle2 - idle1);

    if (totalDelta <= 0.0) return 0.0;

    double usage = (totalDelta - idleDelta) / totalDelta * 100.0;
    if (usage < 0.0) usage = 0.0;
    if (usage > 100.0) usage = 100.0;
    return usage;
}
