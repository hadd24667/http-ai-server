#pragma once

#include <memory>
#include <string>

#include "core/Request.hpp"
#include "core/Socket.hpp"
#include "scheduler/Scheduler.hpp"
#include "threadpool/ThreadPool.hpp"
#include "monitor/Logger.hpp"

class HttpServer {
public:
    HttpServer(int port, int threadCount);
    ~HttpServer();

    void start();
    void stop();

private:
    std::string readRequestBlocking(int clientFd);
    int estimateTaskWorkload(const Request& req);
    int getWeightFromConfig();
    void handleClient(int clientSocketFd, const Request& req);

private:
    int port;
    int threadCount;
    bool isRunning;
    int nextTaskId;

    std::unique_ptr<Socket> serverSocket;

    // IMPORTANT: scheduler phải khai báo trước threadPool
    std::unique_ptr<Scheduler> scheduler;

    std::unique_ptr<ThreadPool> threadPool;
    std::unique_ptr<Logger> logger;

    double latencyAvg;
    std::atomic<std::size_t> pendingTasks{0};
};
