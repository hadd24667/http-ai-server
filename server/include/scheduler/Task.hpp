#pragma once
#include <functional>
#include <iostream>

struct Task {
    int id;
    int estimatedTime;   // SJF
    int weight;          // WFQ
    int remainingTime;   // RR
    std::function<void()> func;

    Task(int id, int est, int w, std::function<void()> f)
        : id(id), estimatedTime(est), weight(w), remainingTime(est), func(f) {}
};
