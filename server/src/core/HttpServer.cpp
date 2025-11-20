#include "core/HttpServer.hpp"
#include "core/HttpParser.hpp"
#include "core/Response.hpp"
#include "core/Socket.hpp"
#include "threadpool/ThreadPool.hpp"
#include "scheduler/Scheduler.hpp"
#include "scheduler/SchedulerFactory.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <iostream>
#include <thread>

// =======================
//  Helpers
// =======================

static void sendAll(int fd, const char* data, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t sent = send(fd, data + total, len - total, 0);
        if (sent <= 0) {
            std::cout << "[SEND] send() failed or client closed\n";
            return;
        }
        total += static_cast<size_t>(sent);
        std::cout << "[SEND] sent=" << sent
                  << " total=" << total
                  << "/" << len << "\n";
    }
}

HttpServer::~HttpServer() = default;

HttpServer::HttpServer(int port, int threadCount)
    : port(port),
      threadCount(threadCount),
      isRunning(false),
      nextTaskId(0)
{
    serverSocket = std::make_unique<Socket>();
    threadPool = std::make_unique<ThreadPool>(threadCount);

    // DÙNG FACTORY CHUẨN — chọn thuật toán theo config
    scheduler = SchedulerFactory::create("FIFO");
}

// Đọc raw HTTP request 1 lần duy nhất
std::string HttpServer::readRequestBlocking(int clientFd) {
    char buffer[4096];
    int bytes = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    std::cout << "[DEBUG] recv bytes = " << bytes << "\n";

    if (bytes <= 0) {
        return "";
    }

    buffer[bytes] = '\0';
    return std::string(buffer);
}

// Ước lượng workload để test SJF / RR / WFQ
int HttpServer::estimateTaskWorkload(const Request& req) {
    // Tạm dùng độ dài path làm workload (càng dài càng nặng)
    int w = static_cast<int>(req.path.size());
    if (w <= 0) w = 1;
    return w;
}

// Lấy weight cho WFQ – tạm cho tất cả = 1
int HttpServer::getWeightFromConfig() {
    return 1;
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

    std::cout << "[SERVER] Accept loop running...\n";

    // Vòng accept: mỗi kết nối -> đọc request -> tạo Task -> scheduler -> threadpool
    while (isRunning) {
        int clientFd = serverSocket->acceptClient();
        if (clientFd < 0) {
            if (isRunning) {
                std::cerr << "[WARN] accept() failed\n";
            }
            continue;
        }

        // Đọc request 1 lần tại đây
        std::string raw = readRequestBlocking(clientFd);
        if (raw.empty()) {
            std::cout << "[DEBUG] empty request, closing client\n";
            close(clientFd);
            continue;
        }

        // Parse request
        Request req = HttpParser::parse(raw);
        std::cout << "[DEBUG] NEW REQUEST, path = [" << req.path << "]\n";

        int est = estimateTaskWorkload(req);
        int weight = getWeightFromConfig();

        int currentTaskId = nextTaskId++;

        Task task(
            currentTaskId,
            est,
            weight,
            [this, clientFd, req, currentTaskId]() {
                std::cout << "[POOL] Executing task id=" << currentTaskId
                          << " path=" << req.path << "\n";
                handleClient(clientFd, req);
            }
        );

        std::cout << "[SCHED] enqueue task id=" << currentTaskId
                  << " est=" << est
                  << " weight=" << weight << "\n";
        scheduler->enqueue(task);

        // Với thiết kế đơn giản hiện tại, ta dequeue ngay
        Task next = scheduler->dequeue();
        std::cout << "[SCHED] dequeue -> task id=" << next.id << "\n";

        threadPool->enqueue(next.func);
    }
}

void HttpServer::stop() {
    isRunning = false;
    serverSocket->closeSocket();
    std::cout << "[SERVER] Stopped.\n";
}

// Giờ acceptLoop không dùng nữa, nhưng nếu header còn khai báo thì có thể để dummy
void HttpServer::acceptLoop() {
    // Không dùng nữa vì đã tích hợp logic vào start()
    // Nếu muốn tách riêng, có thể di chuyển vòng while từ start() sang đây.
}

// handleClient KHÔNG recv, KHÔNG parse lại – chỉ xử lý theo Request đã có
void HttpServer::handleClient(int clientSocketFd, const Request& req) {

    std::cout << "[DEBUG] handleClient START, path=[" << req.path << "]\n";

    // favicon
    if (req.path == "/favicon.ico") {
        Response res;
        res.statusCode = 404;
        res.statusText = "Not Found";
        res.headers["Content-Type"] = "text/plain";
        res.headers["Connection"] = "close";
        res.body = "";

        std::string raw = res.build();
        sendAll(clientSocketFd, raw.c_str(), raw.size());

        std::cout << "[DEBUG] calling shutdown() for favicon\n";
        shutdown(clientSocketFd, SHUT_WR);
        std::cout << "[DEBUG] calling close() for favicon\n";
        close(clientSocketFd);
        std::cout << "[DEBUG] closed client socket (favicon)\n";
        return;
    }

    // Normal response
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
