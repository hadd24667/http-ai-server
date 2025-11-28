#pragma once
#include <functional>
#include <iostream>

struct Task {
    int id;
    int estimatedTime;   // SJF
    int weight;          // WFQ
    int remainingTime;   // RR
    std::function<void()> func;

    Task() : id(0), estimatedTime(1), weight(1), remainingTime(1), func(nullptr) {}

    Task(int id, int est, int w, std::function<void()> f)
        : id(id), estimatedTime(est), weight(w), remainingTime(est), func(f) {}
};
