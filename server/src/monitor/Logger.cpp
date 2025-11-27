#include "monitor/Logger.hpp"
#include <chrono>
#include <iomanip>

Logger::Logger(const std::string& filePath) {
    file.open(filePath, std::ios::out | std::ios::app);
    if (file.tellp() == 0) {
        // ghi header nếu file mới
        file << "timestamp,cpu,queue_len,req_size,algorithm_used,response_time_ms\n";
        file.flush();
    }
}

Logger::~Logger() {
    if (file.is_open()) {
        file.flush();
        file.close();
    }
}

void Logger::log(const LogEntry& e) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!file.is_open()) return;

    file << e.timestamp << ","
         << e.cpu << ","
         << e.queue_len << ","
         << e.req_size << ","
         << e.algorithm_used << ","
         << e.response_time_ms << "\n";
    // có thể không flush mỗi dòng để performance tốt hơn
}
