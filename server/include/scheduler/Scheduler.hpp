#pragma once

#include "Task.hpp"
#include <string>
#include <cstddef>

class Scheduler {
public:
    virtual ~Scheduler() = default;

    // Thêm task mới vào queue
    virtual void enqueue(const Task& task) = 0;

    // Overload có queueLen (dùng cho Adaptive); mặc định bỏ qua
    virtual void enqueue(const Task& task, std::size_t queueLen) {
        (void)queueLen;
        enqueue(task);
    }

    // Lấy task tiếp theo theo chính sách lập lịch
    virtual Task dequeue() = 0;

    // Cho biết scheduler còn task hay không
    virtual bool empty() const = 0;

    // Optional: cho RR (nếu cần)
    virtual void setTimeSlice(int /*ts*/) {}

    // Optional: cho WFQ (nếu cần)
    virtual void updateWeights(int /*newWeight*/) {}

    // Tên thuật toán hiện tại (FIFO/SJF/RR/WFQ/ADAPTIVE...)
    virtual std::string currentAlgorithm() const = 0;
};
