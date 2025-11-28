#include "monitor/Logger.hpp"

Logger::Logger(const std::string& filePath) {
    file.open(filePath, std::ios::out | std::ios::app);
    if (file.tellp() == 0) {
        file << "timestamp,"
        "cpu,"
        "queue_len,"
        "request_method,"
        "request_path_length,"
        "estimated_workload,"
        "algo_at_enqueue,"
        "algo_at_run,"
        "req_size,"
        "response_ms,"
        "latency_avg\n";
    }
}

Logger::~Logger() {
    flush();
    file.close();
}

void Logger::log(const LogEntry& e) {
    std::lock_guard<std::mutex> lock(mtx);

    file << e.timestamp << ","
     << e.cpu << ","
     << e.queue_len << ","
     << e.request_method << ","
     << e.request_path_length << ","
     << e.estimated_workload << ","
     << e.algo_at_enqueue << ","
     << e.algo_at_run << ","
     << e.req_size << ","
     << e.response_time_ms << ","
     << e.prev_latency_avg << "\n";

    counter++;
    if (counter % 50 == 0)
        file.flush();
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mtx);
    file.flush();
}
