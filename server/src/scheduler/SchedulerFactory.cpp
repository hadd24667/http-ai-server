#include "scheduler/SchedulerFactory.hpp"
#include "scheduler/FIFOScheduler.hpp"
#include "scheduler/SJFScheduler.hpp"
#include "scheduler/RRScheduler.hpp"
#include "scheduler/WFQScheduler.hpp"

std::unique_ptr<Scheduler> SchedulerFactory::create(const std::string& type, int timeSlice) {
    if (type == "FIFO") {
        return std::make_unique<FIFOScheduler>();
    }
    if (type == "SJF") {
        return std::make_unique<SJFScheduler>();
    }
    if (type == "RR") {
        return std::make_unique<RRScheduler>(timeSlice);
    }
    if (type == "WFQ") {
        return std::make_unique<WFQScheduler>();
    }

    // Default fallback
    return std::make_unique<FIFOScheduler>();
}
