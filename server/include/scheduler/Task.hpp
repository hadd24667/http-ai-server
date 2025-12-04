#pragma once
#include <functional>
#include <iostream>

struct Task {
    int id;
    int estimatedTime;   // SJF
    int weight;          // WFQ
    int remainingTime;   // RR
    std::string algo_at_enqueue;
    std::function<void()> func;

    Task() : id(0), estimatedTime(1), weight(1), remainingTime(1),algo_at_enqueue("FIFO"), func(nullptr) {}
    
    Task(int id, int est, int w, const std::string& alg, std::function<void()> f)
        : id(id), estimatedTime(est), weight(w),
          remainingTime(est), algo_at_enqueue(alg), func(f) {}
};
