#include "core/HttpServer.hpp"
#include "core/HttpParser.hpp"
#include "core/Response.hpp"
#include "core/Socket.hpp"
#include "threadpool/ThreadPool.hpp"
#include "scheduler/Scheduler.hpp"
#include "scheduler/SchedulerFactory.hpp"
#include "monitor/SystemMetrics.hpp"
#include "monitor/Logger.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <ctime>
#include <fcntl.h>
#include <sstream>
#include <cerrno>
#include <netinet/in.h>
#include <netinet/tcp.h>  // TCP_NODELAY
#include <sys/time.h>      // timeval

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

static std::string nowIso8601() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

HttpServer::~HttpServer() = default;

HttpServer::HttpServer(int port, int threadCount)
    : port(port),
      threadCount(threadCount),
      isRunning(false),
      nextTaskId(0),
      latencyAvg(0.0)
{
    serverSocket = std::make_unique<Socket>();
    threadPool   = std::make_unique<ThreadPool>(threadCount);

    // DÙNG FACTORY CHUẨN — chọn thuật toán theo config
    scheduler = SchedulerFactory::create("ADAPTIVE");
    logger    = std::make_unique<Logger>("data/logs/http_server_log.csv");
}

// Đọc raw HTTP request: loop đến khi có đủ header (\r\n\r\n) hoặc timeout
std::string HttpServer::readRequestBlocking(int clientFd) {
    std::string data;
    char buffer[4096];
    ssize_t n = 0;
    int total = 0;

    // Đặt timeout recv 5s để tránh treo nếu client im lặng
    timeval tv{};
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (true) {
        n = recv(clientFd, buffer, sizeof(buffer), 0);

        if (n < 0) {
            if (errno == EINTR) {
                // bị signal, đọc lại
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cout << "[DEBUG] recv timeout / EAGAIN\n";
                break;
            }
            std::cout << "[DEBUG] recv error: " << std::strerror(errno) << "\n";
            return "";
        }

        if (n == 0) {
            // client đóng kết nối
            break;
        }

        data.append(buffer, n);
        total += static_cast<int>(n);

        // Đã nhận đủ header HTTP
        if (data.find("\r\n\r\n") != std::string::npos) {
            break;
        }

        // Guard: không cho request header quá to
        if (total > 65536) {
            std::cout << "[DEBUG] request too large, total=" << total << "\n";
            break;
        }
    }

    std::cout << "[DEBUG] recv total bytes = " << total << "\n";
    return data;
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
                std::cerr << "[WARN] accept() failed, errno=" << errno << "\n";
            }
            continue;
        }

        // Tắt Nagle cho từng client để giảm latency với request nhỏ
        int flag = 1;
        setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

        // Đọc request từ client
        std::string raw = readRequestBlocking(clientFd);
        if (raw.empty()) {
            std::cout << "[DEBUG] empty or invalid request, closing client\n";
            close(clientFd);
            continue;
        }

        // Parse request
        Request req = HttpParser::parse(raw);
        std::cout << "[DEBUG] NEW REQUEST, path = [" << req.path << "]\n";

        int est    = estimateTaskWorkload(req);
        int weight = getWeightFromConfig();
        int currentTaskId = nextTaskId++;

        auto startTime = std::chrono::steady_clock::now();

        // Lấy thuật toán hiện tại TRƯỚC KHI enqueue (algo_at_enqueue)
        std::string algo_enqueue = scheduler->currentAlgorithm();

        // ===== TẠO TASK =====
        Task task(
            currentTaskId,
            est,
            weight,
            // capture đầy đủ: this, clientFd, req, startTime, est, algo_enqueue
            [this, clientFd, req, startTime, est, algo_enqueue]() {
                auto t0 = startTime;

                // Xử lý request
                handleClient(clientFd, req);

                auto t1 = std::chrono::steady_clock::now();
                double respMs =
                    std::chrono::duration<double, std::milli>(t1 - t0).count();

                // Cập nhật moving average latency (EWMA)
                this->latencyAvg = this->latencyAvg * 0.9 + respMs * 0.1;

                double       cpu      = SystemMetrics::getCpuUsage();
                std::size_t  queueLen = this->threadPool->getPendingTaskCount();
                std::size_t  reqSize  = req.path.size();  // hoặc raw.size()

                // Thuật toán thực sự đang active tại thời điểm RUN
                std::string algo_run = this->scheduler->currentAlgorithm();

                if (this->logger) {
                    LogEntry e;
                    e.timestamp            = nowIso8601();
                    e.cpu                  = cpu;
                    e.queue_len            = queueLen;

                    e.request_method       = req.method;      // ví dụ: "GET"
                    e.request_path_length  = req.path.size(); // độ dài path
                    e.estimated_workload   = est;             // từ estimateTaskWorkload

                    e.algo_at_enqueue      = algo_enqueue;    // algo lúc enqueue
                    e.algo_at_run          = algo_run;        // algo lúc run

                    e.req_size             = reqSize;
                    e.response_time_ms     = respMs;
                    e.prev_latency_avg     = this->latencyAvg;

                    this->logger->log(e);
                }

                std::cout << "[LOG] cpu=" << cpu
                          << " q=" << queueLen
                          << " algo_enqueue=" << algo_enqueue
                          << " algo_run=" << algo_run
                          << " rt=" << respMs << "ms"
                          << " latAvg=" << this->latencyAvg << "ms\n";
            }
        );

        std::cout << "[SCHED] enqueue task id=" << currentTaskId
                  << " est=" << est
                  << " weight=" << weight << "\n";

        // Lấy queueLen hiện tại của ThreadPool để đưa cho AdaptiveScheduler
        std::size_t qlen = threadPool->getPendingTaskCount();
        scheduler->enqueue(task, qlen);

        // Với thiết kế đơn giản hiện tại, ta dequeue ngay 1 task và đưa cho ThreadPool
        Task next = scheduler->dequeue();
        std::cout << "[SCHED] dequeue -> task id=" << next.id << "\n";

        // ThreadPool dùng Task
        threadPool->enqueue(next);
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
