#pragma once
#include "Scheduler.hpp"
#include <memory>
#include <string>

class SchedulerFactory {
public:
    static std::unique_ptr<Scheduler> create(const std::string& type, int timeSlice = 1);
};
