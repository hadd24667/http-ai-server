#include "scheduler/SchedulerFactory.hpp"
#include "scheduler/FIFOScheduler.hpp"
#include "scheduler/SJFScheduler.hpp"
#include "scheduler/RRScheduler.hpp"
#include "scheduler/WFQScheduler.hpp"
#include "scheduler/AdaptiveScheduler.hpp"

#include <algorithm>
#include <iostream>


std::unique_ptr<Scheduler> SchedulerFactory::create(const std::string& algoName) {
    std::string name = algoName;
    
    // lowercase để dễ so sánh
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    std::cout << "[SchedulerFactory] Creating scheduler: " << name << "\n";

    if (name == "fifo") {
        return std::make_unique<FIFOScheduler>();
    }
    if (name == "sjf") {
        return std::make_unique<SJFScheduler>();
    }
    if (name == "rr" || name == "roundrobin") {
        return std::make_unique<RRScheduler>();
    }
    if (name == "wfq") {
        return std::make_unique<WFQScheduler>();
    }
    if (name == "adaptive") {
        return std::make_unique<AdaptiveScheduler>();
    }

    std::cout << "[SchedulerFactory] Unknown algo '" << algoName 
              << "', fallback = ADAPTIVE\n";

    return std::make_unique<AdaptiveScheduler>();
}
