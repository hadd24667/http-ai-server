#include "monitor/SystemMetrics.hpp"
#include <thread>
#include <fstream>
#include <chrono>
#include <iostream>

std::atomic<double> SystemMetrics::CPU_CACHE{0.0};
std::atomic<double> SystemMetrics::BUSY_ACCUM{0.0};
std::mutex          SystemMetrics::busyMutex;

// Đọc CPU từ /proc/stat (Linux / WSL)
static double computeCpu() {
    std::ifstream f("/proc/stat");
    if (!f.is_open()) return 0.0;

    std::string cpu;
    long u, n, s, i, iw, ir, sf, st;
    f >> cpu >> u >> n >> s >> i >> iw >> ir >> sf >> st;

    static long prevTotal = 0;
    static long prevIdle  = 0;

    long idle    = i + iw;
    long nonIdle = u + n + s + ir + sf + st;
    long total   = idle + nonIdle;

    long totalDelta = total - prevTotal;
    long idleDelta  = idle  - prevIdle;

    prevTotal = total;
    prevIdle  = idle;

    if (totalDelta <= 0) return 0.0;

    double usage = (double)(totalDelta - idleDelta) / (double)totalDelta * 100.0;
    if (usage < 0.0)   usage = 0.0;
    if (usage > 100.0) usage = 100.0;
    return usage;
}

void SystemMetrics::init() {
    // Thread nền: mỗi 200ms đọc CPU OS + snapshot busy-time để tạo CPU tổng hợp
    std::thread([](){
        using namespace std::chrono;
        constexpr double WINDOW_SEC = 0.2;  // 200ms

        while (true) {
            double osCpu = computeCpu();

            double busySec = 0.0;
            {
                std::lock_guard<std::mutex> lock(busyMutex);
                busySec = BUSY_ACCUM.load();
                BUSY_ACCUM.store(0.0);
            }

            double busyPercent = (busySec / WINDOW_SEC) * 100.0;
            if (busyPercent < 0.0)   busyPercent = 0.0;
            if (busyPercent > 100.0) busyPercent = 100.0;

            // Kết hợp CPU OS và busy-time (50/50)
            double combined = (osCpu * 0.5) + (busyPercent * 0.5);
            if (combined < 0.0)   combined = 0.0;
            if (combined > 100.0) combined = 100.0;

            CPU_CACHE.store(combined);

            std::this_thread::sleep_for(milliseconds(200));
        }
    }).detach();
}

void SystemMetrics::addBusy(double seconds) {
    if (seconds <= 0.0) return;
    std::lock_guard<std::mutex> lock(busyMutex);
    double cur = BUSY_ACCUM.load();
    BUSY_ACCUM.store(cur + seconds);
}

double SystemMetrics::getCpuUsage() {
    return CPU_CACHE.load();
}
