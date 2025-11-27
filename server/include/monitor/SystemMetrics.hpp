#pragma once
#include <cstdint>

class SystemMetrics {
public:
    // Trả về CPU usage (%) trong khoảng 0–100
    static double getCpuUsage();
};
