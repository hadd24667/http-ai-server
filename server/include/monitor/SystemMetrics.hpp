#pragma once
#include <atomic>
#include <mutex>

class SystemMetrics {
public:
    // Gọi 1 lần lúc khởi động (main.cpp)
    static void init();

    // CPU tổng hợp: kết hợp giữa CPU OS (/proc/stat) và busy-time đo được
    static double getCpuUsage();

    // Gọi từ SmartLoad để cộng thêm thời gian CPU bận (tính bằng giây)
    static void addBusy(double seconds);

private:
    // CPU (%) đã tính sẵn, cache lại để đọc nhanh
    static std::atomic<double> CPU_CACHE;

    // Tổng busy-time (giây) tích lũy trong 1 window (200ms)
    static std::atomic<double> BUSY_ACCUM;

    static std::mutex busyMutex;
};
