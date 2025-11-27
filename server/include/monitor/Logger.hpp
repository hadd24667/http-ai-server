#pragma once
#include <string>
#include <mutex>
#include <fstream>

struct LogEntry {
    std::string timestamp;       // ISO-8601 hoặc đơn giản là epoch ms
    double cpu = 0.0;
    std::size_t queue_len = 0;
    std::size_t req_size = 0;
    std::string algorithm_used;
    double response_time_ms = 0.0;
};

class Logger {
public:
    explicit Logger(const std::string& filePath);
    ~Logger();

    void log(const LogEntry& entry);

private:
    std::mutex mtx;
    std::ofstream file;
};
