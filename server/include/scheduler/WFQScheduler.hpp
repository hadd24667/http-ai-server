#pragma once
#include "Scheduler.hpp"
#include <queue>
#include <cmath>
#include <iostream>

class WFQScheduler : public Scheduler {
private:
    struct Node {
        Task task;
        double finishTime;
    };

    struct Compare {
        bool operator()(const Node& a, const Node& b) const {
            return a.finishTime > b.finishTime;
        }
    };

    double virtualTime = 0;
    std::priority_queue<Node, std::vector<Node>, Compare> pq;

public:
    void enqueue(const Task& task) override {
                double finish = virtualTime + (double)task.estimatedTime / task.weight;
                std::cout << "[SCHED] enqueue WFQ id=" << task.id 
                    << " weight=" << task.weight 
                    << " finishTime=" << finish << "\n";
                pq.push({task, finish});
    }

    Task dequeue() override {
        Node n = pq.top();
        pq.pop();
        virtualTime = n.finishTime;
        std::cout << "[SCHED] dequeue WFQ -> task id=" << n.task.id << "\n";
        return n.task;
    }
};
