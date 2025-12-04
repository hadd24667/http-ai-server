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
#include <errno.h>
#include <string.h>
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
        // MSG_NOSIGNAL: không bắn SIGPIPE nếu client đã đóng
        ssize_t sent = send(fd, data + total, len - total, MSG_NOSIGNAL);
        if (sent < 0) {
            std::cout << "[SEND] send() failed: errno=" << errno
                      << " (" << std::strerror(errno) << ")\n";
            break;
        }
        if (sent == 0) {
            std::cout << "[SEND] send() returned 0, client closed?\n";
            break;
        }

        total += static_cast<size_t>(sent);
        // Nếu log nhiều quá muốn nhẹ hơn thì comment block này đi
        // std::cout << "[SEND] sent=" << sent << " total=" << total << "/" << len << "\n";
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

    // 1) Tạo scheduler trước
    scheduler = SchedulerFactory::create("ADAPTIVE");

    // 2) ThreadPool nhận con trỏ scheduler – pull-mode
    threadPool = std::make_unique<ThreadPool>(threadCount, scheduler.get());

    // 3) logger
    logger = std::make_unique<Logger>("data/logs/http_server_log.csv");
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

        // ===== TẠO TASK =====

        // 1) Tăng pendingTasks TẠI THỜI ĐIỂM ENQUEUE
        std::size_t qLenAtEnqueue = threadPool->incrementPendingTasks();

        // 2) Lấy thuật toán tại enqueue
        std::string algo_enqueue = scheduler->currentAlgorithm();

        // 3) Tạo Task
        Task task(
            currentTaskId,
            est,
            weight,
            algo_enqueue,
            [this, clientFd, req, startTime, est, algo_enqueue, qLenAtEnqueue]() {

                auto t0 = startTime;

                // Xử lý request
                handleClient(clientFd, req);

                auto t1 = std::chrono::steady_clock::now();
                double respMs =
                    std::chrono::duration<double, std::milli>(t1 - t0).count();

                // EWMA latency
                this->latencyAvg = this->latencyAvg * 0.9 + respMs * 0.1;

                double       cpu      = SystemMetrics::getCpuUsage();
                std::size_t  reqSize  = req.path.size();

                // ✨ GIỮ NGUYÊN queue_len TẠI ENQUEUE (không lấy pendingTasks ở đây)
                std::size_t queueLen = qLenAtEnqueue;

                std::string algo_run = this->scheduler->currentAlgorithm();

                if (this->logger) {
                    LogEntry e;
                    e.timestamp            = nowIso8601();
                    e.cpu                  = cpu;
                    e.queue_len            = queueLen;            // ✔ QUEUE LEN ĐÚNG
                    e.request_method       = req.method;
                    e.request_path_length  = req.path.size();
                    e.estimated_workload   = est;
                    e.algo_at_enqueue      = algo_enqueue;
                    e.algo_at_run          = algo_run;
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

        // == CHỈ GỌI 1 LẦN ==
        // KHÔNG pendingTasks.fetch_add() ở đây nữa

        // ==== gọi enqueue đúng:
        scheduler->enqueue(task, qLenAtEnqueue);
        threadPool->notifyWorker();       // <-- Đánh thức worker tại đây

    }
}

void HttpServer::stop() {
    isRunning = false;
    serverSocket->closeSocket();
    std::cout << "[SERVER] Stopped.\n";
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

        // =======================
    //  SMART WORKLOAD ENGINE
    // =======================

    // tăng hệ số để CPU dễ chạm 50–90%
    static double loadFactor = 20000.0;  // khởi đầu
    int w = std::max(1, (int)req.path.size());

    volatile long dummy = 0;
    long iterations = (long)(w * loadFactor);

    // đo thời gian CPU bận cho workload này
    auto t0 = std::chrono::steady_clock::now();

    for (long i = 0; i < iterations; ++i) {
        dummy += i;
    }

    auto t1 = std::chrono::steady_clock::now();
    double busySec = std::chrono::duration<double>(t1 - t0).count();

    // báo busy-time cho SystemMetrics
    SystemMetrics::addBusy(busySec);

    // lấy CPU tổng hợp
    double cpu = SystemMetrics::getCpuUsage();

    // Nếu CPU quá thấp → tăng loadFactor
    if (cpu < 30.0) {
        loadFactor *= 1.10;  // tăng nhẹ 10%
    }
    // Nếu CPU quá cao → giảm loadFactor
    else if (cpu > 85.0) {
        loadFactor *= 0.85;  // giảm 15%
    }

    // Giới hạn loadFactor để tránh quá to / quá nhỏ
    if (loadFactor < 5000.0)     loadFactor = 5000.0;
    if (loadFactor > 200000.0)   loadFactor = 200000.0;

    std::cout << "[LOAD] cpu=" << cpu
              << " loadFactor=" << loadFactor
              << " pathLen=" << w
              << " busySec=" << busySec << "\n";

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
