#include "core/HttpServer.hpp"
#include "core/HttpParser.hpp"
#include "core/Response.hpp"
#include "core/Socket.hpp"
#include "threadpool/ThreadPool.hpp"
#include "scheduler/Scheduler.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <iostream>
#include <thread>

HttpServer::~HttpServer() = default;

HttpServer::HttpServer(int port, int threadCount)
    : port(port),
      threadCount(threadCount),
      isRunning(false)
{
    serverSocket = std::make_unique<Socket>();
    threadPool = std::make_unique<ThreadPool>(threadCount);
    // Scheduler tạm thời lấy FIFO, sau này thay bằng Adaptive
    scheduler = std::unique_ptr<Scheduler>(Scheduler::createDefault());
}

void HttpServer::start() {
    std::cout << "[SERVER] Starting on port " << port << "...\n";

    if (!serverSocket->bind(port)) {
        std::cerr << "[ERROR] Cannot bind port " << port << "\n";
        return;
    }

    if (!serverSocket->listen()) {
        std::cerr << "[ERROR] Listen failed\n";
        return;
    }

    isRunning = true;

    while (isRunning) {
    int clientFd = serverSocket->acceptClient();
    if (clientFd >= 0) {
        threadPool->enqueue([this, clientFd] {
            handleClient(clientFd);
        });
    }
}

}

void HttpServer::stop() {
    isRunning = false;
    serverSocket->closeSocket();
    std::cout << "[SERVER] Stopped.\n";
}
    
void HttpServer::acceptLoop() {
    std::cout << "[SERVER] Accept loop running...\n";

    while (isRunning) {
        int clientFd = serverSocket->acceptClient();

        if (clientFd < 0) {
            if (isRunning) std::cerr << "[WARN] accept() failed.\n";
            continue;
        }

        // Đưa vào thread pool xử lý
        threadPool->enqueue([this, clientFd]() {
            handleClient(clientFd);
        });
    }
}

void sendAll(int fd, const char* data, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t sent = send(fd, data + total, len - total, 0);
        std::cout << "[SEND] sent=" << sent
                  << " total=" << total+sent 
                  << "/" << len << "\n";
        if (sent <= 0) return;
        total += sent;
    }
}


void HttpServer::handleClient(int clientSocketFd) {

    std::cout << "[DEBUG] handleClient START\n";
    std::cout << "[DEBUG] waiting recv...\n";
    char buffer[4096];
    int bytes = recv(clientSocketFd, buffer, sizeof(buffer) - 1, 0);
    std::cout << "[DEBUG] recv bytes = " << bytes << "\n";

    if (bytes <= 0) {
        close(clientSocketFd);
        return;
    }

    buffer[bytes] = '\0';
    std::string rawRequest(buffer);

    // Parse request BEFORE checking path
    Request req = HttpParser::parse(rawRequest);

    std::cout << "REQUEST PATH: [" << req.path << "]\n";
    std::cout << "[DEBUG] RAW REQUEST BYTES: ";
    for (int i = 0; i < bytes; i++) {
        std::cout << (int)(unsigned char)buffer[i] << " ";
    }
    std::cout << "\n";


    // ALWAYS handle favicon
    if (req.path == "/favicon.ico") {
        Response res;
        res.statusCode = 404;
        res.statusText = "Not Found";
        res.headers["Content-Type"] = "text/plain";
        res.headers["Connection"] = "close";
        res.body = "";

        std::string raw = res.build();
        send(clientSocketFd, raw.c_str(), raw.size(), 0);
        close(clientSocketFd);
        return;
    }

    // Build normal response
    Response res;
    res.statusCode = 200;
    res.statusText = "OK";
    res.headers["Content-Type"] = "text/plain";
    res.headers["Connection"] = "close";
    res.body = "You requested: " + req.path;

    std::string rawResponse = res.build();

    std::cout << "===== RAW RESPONSE =====\n" 
              << rawResponse 
              << "\n========================\n";

    sendAll(clientSocketFd, rawResponse.c_str(), rawResponse.size());

    std::cout << "[DEBUG] calling shutdown()\n";
    shutdown(clientSocketFd, SHUT_WR);
    std::cout << "[DEBUG] calling close()\n";
    close(clientSocketFd);
    std::cout << "[DEBUG] closed client socket\n";

}


