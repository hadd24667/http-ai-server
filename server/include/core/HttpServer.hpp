#pragma once

#include <memory>
#include <string>
#include "core/Request.hpp"

class Socket;
class Scheduler;
class ThreadPool;
class Logger;

// Forward declaration cho OpenSSL
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

class HttpServer {
public:
    HttpServer(int port, int threadCount);
    ~HttpServer();

    void start();
    void stop();

private:
    int    port;
    int    threadCount;
    bool   isRunning;
    int    nextTaskId;
    double latencyAvg;

    std::unique_ptr<Socket>    serverSocket;
    std::unique_ptr<Scheduler> scheduler;
    std::unique_ptr<ThreadPool> threadPool;
    std::unique_ptr<Logger>    logger;

    // SSL context cho HTTPS
    SSL_CTX* sslCtx;

private:
    // Đọc HTTP request qua SSL
    std::string readRequestBlockingSSL(SSL* ssl);

    // Ước lượng workload cho scheduler
    int estimateTaskWorkload(const Request& req);
    int getWeightFromConfig();

    // Static files, router và handler
    bool serveStaticFile(SSL* ssl, int clientFd, const std::string& path);
    void handleClient(SSL* ssl, int clientSocketFd, const Request& req);

    void handleGET(SSL* ssl, int clientFd, const Request& req);
    void handlePOST(SSL* ssl, int clientFd, const Request& req);
    void handlePUT(SSL* ssl, int clientFd, const Request& req);
    void handleDELETE(SSL* ssl, int clientFd, const Request& req);
};
