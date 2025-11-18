#pragma once
#include <iostream>
#include <string>
#include <atomic>
#include <memory>
#include "utils/Config.hpp"

class ThreadPool;
class Scheduler;
class Socket;

class HttpServer {
    public:
        HttpServer(int port, int threadCount);
        ~HttpServer();

        // Start server
        void start();

        // stop server
        void stop();
    
    private:
        int port;
        int threadCount;

        std::unique_ptr<ThreadPool> threadPool;
        std::unique_ptr<Scheduler> scheduler;
        std::unique_ptr<Socket> serverSocket;
        
        std::atomic<bool> isRunning;

        // accept new connections from clients
        void acceptLoop();

        // handle 1 rewuest from client (worker thread)
        void handleClient(int clientSocket);
};