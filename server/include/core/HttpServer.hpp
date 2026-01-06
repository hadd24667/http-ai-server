#pragma once

#include <memory>
#include <string>
#include "core/Request.hpp"
#include "core/Response.hpp"

class Socket;
class Scheduler;
class ThreadPool;
class Logger;

// Forward declaration cho OpenSSL
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

class HttpServer {
public:
    HttpServer(int port, int threadCount, const std::string& algo);
    ~HttpServer();

    void start();
    void stop();

private:
    int    port;
    int    threadCount;
    bool   isRunning;
    int    nextTaskId;
    double latencyAvg;
    std::string algoName;

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

    // Static files, router và handler
    bool serveStaticFile(Response& res, const std::string& path);
    void handleClient(SSL* ssl, int clientSocketFd, const Request& req);

    void handleGET(Response& res, const Request& req);
    void handlePOST(Response& res, const Request& req);
    void handlePUT(Response& res, const Request& req);
    void handleDELETE(Response& res, const Request& req);

};
