#pragma once
#include <string>
#include <mutex>
#include <fstream>

struct LogEntry {
    std::string timestamp;

    double cpu;
    std::size_t queue_len;

    std::string request_method;
    std::size_t request_path_length;
    int estimated_workload;

    std::string algo_at_enqueue;
    std::string algo_at_run;

    std::size_t req_size;
    double response_time_ms;
    double prev_latency_avg;
};

class Logger {
public:
    Logger(const std::string& filePath);
    ~Logger();

    void log(const LogEntry& e);
    void flush();

private:
    std::ofstream file;
    std::mutex mtx;
    int counter = 0;
};
