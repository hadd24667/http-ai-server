// Task.hpp
#pragma once
#include <functional>
#include <string>
#include <cstddef>

struct Task {
    std::size_t id{};
    int estimatedTime{};
    int remainingTime{}; 
    int weight{};
    std::string algoAtEnqueue;

    std::string request_method;
    int request_path_length{};
    std::size_t req_size{};

    std::function<void()> fn;

    Task() = default;

    Task(
        std::size_t id_,
        int est_,
        int weight_,
        std::string algo_,
        std::string method_,
        int pathLen_,
        std::size_t reqSize_,
        std::function<void()> fn_
    )
        : id(id_),
          estimatedTime(est_),
          remainingTime(est_),
          weight(weight_),
          algoAtEnqueue(std::move(algo_)),
          request_method(std::move(method_)),
          request_path_length(pathLen_),
          req_size(reqSize_),
          fn(std::move(fn_)) {}
};

