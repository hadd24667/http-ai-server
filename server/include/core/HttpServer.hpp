#pragma once

#include <memory>
#include <string>

#include "core/Request.hpp"
#include "core/Socket.hpp"
#include "scheduler/Scheduler.hpp"
#include "threadpool/ThreadPool.hpp"
#include "monitor/Logger.hpp"

class HttpServer {
    std::unique_ptr<Logger> logger;
public:
    HttpServer(int port, int threadCount);
    ~HttpServer();

    void start();
    void stop();

private:
    int port;
    int threadCount;
    bool isRunning;
    int nextTaskId;
    double latencyAvg = 0.0;


    std::unique_ptr<Socket> serverSocket;
    std::unique_ptr<ThreadPool> threadPool;
    std::unique_ptr<Scheduler> scheduler;

    // ===== Helper functions =====
    std::string readRequestBlocking(int clientFd);
    int estimateTaskWorkload(const Request& req);
    int getWeightFromConfig();

    void handleClient(int clientSocketFd, const Request& req);

    // Not used anymore, but keep to avoid linker errors
    void acceptLoop();
};
