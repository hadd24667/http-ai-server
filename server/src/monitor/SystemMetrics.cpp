#include "monitor/SystemMetrics.hpp"
#include <thread>
#include <fstream>
#include <chrono>

std::atomic<double> SystemMetrics::CPU_CACHE{0.0};

static double computeCpu() {
    std::ifstream f("/proc/stat");
    if (!f.is_open()) return 0;

    std::string cpu;
    long u,n,s,i,iw,ir,sf,st;
    f >> cpu >> u >> n >> s >> i >> iw >> ir >> sf >> st;

    static long prevTotal = 0, prevIdle = 0;
    long idle = i + iw;
    long nonIdle = u + n + s + ir + sf + st;
    long total = idle + nonIdle;

    long totalDelta = total - prevTotal;
    long idleDelta = idle - prevIdle;

    prevTotal = total;
    prevIdle = idle;

    if (totalDelta <= 0) return 0.0;
    return (double)(totalDelta - idleDelta) / totalDelta * 100.0;
}

void SystemMetrics::init() {
    std::thread([](){
        while (true) {
            CPU_CACHE.store(computeCpu());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }).detach();
}

double SystemMetrics::getCpuUsage() {
    return CPU_CACHE.load();
}
