#pragma once
#include <optional>
#include <string>
#include <cstddef>

struct AIFeatures {
    double cpu;
    std::size_t queue_len;
    int queue_bin;
    std::string request_method;
    int request_path_length;
    double estimated_workload;
    std::size_t req_size;
};

class AIClient {
public:
    AIClient(std::string url, long timeoutMs);
    std::optional<std::string> predict(const AIFeatures& f) const;

private:
    std::string url_;
    long timeoutMs_;
};
