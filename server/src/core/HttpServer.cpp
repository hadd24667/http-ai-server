#include "core/HttpServer.hpp"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include "core/HttpParser.hpp"
#include "core/Response.hpp"
#include "core/Socket.hpp"
#include "monitor/Logger.hpp"
#include "monitor/SystemMetrics.hpp"
#include "scheduler/Scheduler.hpp"
#include "scheduler/SchedulerFactory.hpp"
#include "threadpool/ThreadPool.hpp"

// OpenSSL
#include <openssl/err.h>
#include <openssl/ssl.h>

// =======================
//  Helpers
// =======================
static inline long long nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

#define LOGX(tag, msg)                                        \
    std::cout << "[" << nowMs() << "ms]"                      \
              << "[TID " << std::this_thread::get_id() << "]" \
              << "[" << tag << "] " << msg << std::endl;
// Gửi toàn bộ buffer qua SSL
static void sendAllSSL(SSL* ssl, const char* data, size_t len) {
    size_t total = 0;

    while (total < len) {
        size_t remain = len - total;
        int sent = SSL_write(ssl, data + total, static_cast<int>(remain));
        if (sent <= 0) {
            int err = SSL_get_error(ssl, sent);

            if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
                // retry nhẹ
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            LOGX("SEND", "❌ SSL_write fatal");
            ERR_print_errors_fp(stderr);
            break;
        }

        total += static_cast<size_t>(sent);
    }
}

// Thời gian ISO8601
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

// endsWith cho C++17
static bool endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

HttpServer::HttpServer(int port, int threadCount, const std::string& algo)
    : port(port),
      threadCount(threadCount),
      isRunning(false),
      nextTaskId(0),
      latencyAvg(0.0),
      sslCtx(nullptr),
      algoName(algo) {
    serverSocket = std::make_unique<Socket>();

    // 1) Tạo scheduler
    scheduler = SchedulerFactory::create(algoName);

    // 2) ThreadPool nhận scheduler – pull-mode
    threadPool = std::make_unique<ThreadPool>(threadCount, scheduler.get());

    // 3) logger
    logger = std::make_unique<Logger>("data/logs/http_server_log.csv");

    // 4) Khởi tạo OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    sslCtx = SSL_CTX_new(TLS_server_method());
    if (!sslCtx) {
        std::cerr << "[SSL] Failed to create SSL_CTX\n";
        ERR_print_errors_fp(stderr);
    } else {
        // Load certificate và private key
        if (SSL_CTX_use_certificate_file(sslCtx, "cert/server.crt", SSL_FILETYPE_PEM) <= 0 ||
            SSL_CTX_use_PrivateKey_file(sslCtx, "cert/server.key", SSL_FILETYPE_PEM) <= 0) {
            std::cerr << "[SSL] Failed to load certificate or private key\n";
            ERR_print_errors_fp(stderr);
            SSL_CTX_free(sslCtx);
            sslCtx = nullptr;
        }
    }
}

HttpServer::~HttpServer() {
    if (sslCtx) {
        SSL_CTX_free(sslCtx);
        sslCtx = nullptr;
    }
}

// Đọc raw HTTP request qua SSL: đến khi có "\r\n\r\n" hoặc quá lớn / lỗi
std::string HttpServer::readRequestBlockingSSL(SSL* ssl) {
    std::string data;
    char buffer[4096];
    int total = 0;

    while (true) {
        int n = SSL_read(ssl, buffer, sizeof(buffer));

        if (n <= 0) {
            int err = SSL_get_error(ssl, n);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                continue;  // thử lại
            }
            std::cout << "[DEBUG] SSL_read error, err=" << err << "\n";
            ERR_print_errors_fp(stderr);
            break;
        }

        data.append(buffer, n);
        total += n;

        // Đã nhận đủ header HTTP
        if (data.find("\r\n\r\n") != std::string::npos) {
            break;
        }

        // Guard: không cho request header quá lớn
        if (total > 65536) {
            std::cout << "[DEBUG] request too large, total=" << total << "\n";
            break;
        }
    }

    std::cout << "[DEBUG] SSL recv total bytes = " << total << "\n";
    return data;
}

// Ước lượng workload để test SJF / RR / WFQ
int HttpServer::estimateTaskWorkload(const Request& req) {
    int w = static_cast<int>(req.path.size());
    if (w <= 0) w = 1;
    return w;
}

// Lấy weight cho WFQ – tạm cho tất cả = 1
int HttpServer::getWeightFromConfig() { return 1; }

void HttpServer::start() {
    std::cout << "[SERVER] Starting on port " << port << "...\n";

    if (!sslCtx) {
        std::cerr << "[ERROR] SSL_CTX not initialized. HTTPS cannot start.\n";
        return;
    }

    if (!serverSocket->bind(port)) {
        std::cerr << "[ERROR] Cannot bind port " << port << "\n";
        return;
    }

    if (!serverSocket->listen()) {
        std::cerr << "[ERROR] Listen failed\n";
        return;
    }

    isRunning = true;

    std::cout << "[SERVER] HTTPS Accept loop running...\n";

    // Vòng accept: mỗi kết nối -> SSL handshake -> đọc request -> Task -> scheduler -> threadpool
    while (isRunning) {
        int clientFd = serverSocket->acceptClient();

        LOGX("NET", "Set SO_SNDTIMEO/SO_RCVTIMEO = 5s");

        if (clientFd < 0) {
            if (isRunning) {
                std::cerr << "[WARN] accept() failed, errno=" << errno << "\n";
            }
            continue;
        }

        // Tắt Nagle cho client để giảm latency
        int flag = 1;
        setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
        // timeout send/recv để tránh treo vô hạn
        timeval tv{};
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(clientFd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        // Tạo SSL object cho client
        SSL* ssl = SSL_new(sslCtx);
        if (!ssl) {
            std::cerr << "[SSL] SSL_new failed\n";
            close(clientFd);
            continue;
        }
        SSL_set_fd(ssl, clientFd);

        int sslAcceptRet = SSL_accept(ssl);
        if (sslAcceptRet <= 0) {
            std::cerr << "[SSL] SSL_accept failed\n";
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(clientFd);
            continue;
        }

        // Đọc request từ client qua SSL
        std::string raw = readRequestBlockingSSL(ssl);
        if (raw.empty()) {
            std::cout << "[DEBUG] empty or invalid request, closing client\n";
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(clientFd);
            continue;
        }

        // Parse request
        Request req = HttpParser::parse(raw);
        std::cout << "[DEBUG] NEW REQUEST, path = [" << req.path << "]\n";

        std::cout << "[TRACE] after-parse: about to estimate workload\n";

        int est = estimateTaskWorkload(req);
        int weight = getWeightFromConfig();
        int currentTaskId = nextTaskId++;

        std::cout << "[TRACE] after-parse: est=" << est << " weight=" << weight
                  << " taskId=" << currentTaskId << "\n";

        auto startTime = std::chrono::steady_clock::now();

        std::cout << "[TRACE] before incrementPendingTasks()\n";
        std::size_t qLenAtEnqueue = threadPool->incrementPendingTasks();
        std::cout << "[TRACE] after incrementPendingTasks(): qLenAtEnqueue=" << qLenAtEnqueue
                  << "\n";

        std::cout << "[TRACE] before scheduler->currentAlgorithm()\n";
        std::string algo_enqueue = scheduler->currentAlgorithm();
        std::cout << "[TRACE] after scheduler->currentAlgorithm(): " << algo_enqueue << "\n";

        // 3) Tạo Task
        std::string method = req.method;
        int pathLen = static_cast<int>(req.path.size());

        // req_size: ưu tiên body, fallback path
        std::size_t reqSize = req.body.size();
        if (reqSize == 0) reqSize = req.path.size();

        Task task(currentTaskId, est, weight, algo_enqueue,
                  method,   // request_method
                  pathLen,  // request_path_length
                  reqSize,  // req_size
                  [this, ssl, clientFd, req, startTime, est, algo_enqueue, qLenAtEnqueue]() {
                      std::this_thread::sleep_for(std::chrono::milliseconds(10));

                      auto t0 = startTime;

                      // Xử lý request (đã có SSL + FD)
                      handleClient(ssl, clientFd, req);

                      auto t1 = std::chrono::steady_clock::now();
                      double respMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

                      // EWMA latency
                      this->latencyAvg = this->latencyAvg * 0.9 + respMs * 0.1;

                      double cpu = SystemMetrics::getCpuUsage();
                      std::size_t reqSize = req.path.size();
                      std::size_t queueLen = qLenAtEnqueue;
                      std::string algo_run = this->scheduler->currentAlgorithm();

                      if (this->logger) {
                          LogEntry e;
                          e.queue_len = queueLen;
                          e.timestamp = nowIso8601();
                          e.cpu = cpu;
                          e.request_method = req.method;
                          e.request_path_length = req.path.size();
                          e.estimated_workload = est;
                          e.algo_at_enqueue = algo_enqueue;
                          e.algo_at_run = algo_run;
                          e.req_size = reqSize;
                          e.response_time_ms = respMs;
                          e.prev_latency_avg = this->latencyAvg;

                          this->logger->log(e);
                      }

                      std::cout << "[LOG] cpu=" << cpu << " q=" << queueLen
                                << " algo_enqueue=" << algo_enqueue << " algo_run=" << algo_run
                                << " rt=" << respMs << "ms"
                                << " latAvg=" << this->latencyAvg << "ms\n";
                  });

        // enqueue
        std::cout << "[TRACE] before scheduler->enqueue(task)\n";
        scheduler->enqueue(task, qLenAtEnqueue);
        std::cout << "[TRACE] after scheduler->enqueue(task)\n";

        std::cout << "[TRACE] before threadPool->notifyWorker()\n";
        threadPool->notifyWorker();
        std::cout << "[TRACE] after threadPool->notifyWorker()\n";
    }
}

void HttpServer::stop() {
    isRunning = false;
    serverSocket->closeSocket();
    std::cout << "[SERVER] Stopped.\n";
}

// =======================
// Static file handler
// =======================
bool HttpServer::serveStaticFile(Response& res, const std::string& path) {
    std::string fullPath = "www" + path;
    if (path == "/") fullPath = "www/index.html";

    std::ifstream f(fullPath, std::ios::binary);
    if (!f.good()) return false;

    std::ostringstream ss;
    ss << f.rdbuf();
    res.body = ss.str();

    res.statusCode = 200;
    res.statusText = "OK";

    // MIME types
    if (endsWith(fullPath, ".html"))
        res.headers["Content-Type"] = "text/html";
    else if (endsWith(fullPath, ".css"))
        res.headers["Content-Type"] = "text/css";
    else if (endsWith(fullPath, ".js"))
        res.headers["Content-Type"] = "application/javascript";
    else if (endsWith(fullPath, ".png"))
        res.headers["Content-Type"] = "image/png";
    else if (endsWith(fullPath, ".jpg") || endsWith(fullPath, ".jpeg"))
        res.headers["Content-Type"] = "image/jpeg";
    else
        res.headers["Content-Type"] = "application/octet-stream";

    return true;
}

// =======================
// 4 handler method
// =======================
void HttpServer::handleGET(Response& res, const Request& req) {
    res.statusCode = 200;
    res.statusText = "OK";
    res.body = "GET " + req.path;
}

void HttpServer::handlePOST(Response& res, const Request& req) {
    res.statusCode = 200;
    res.statusText = "OK";
    res.headers["Content-Type"] = "application/json";
    res.body = "{ \"received\": \"" + req.body + "\" }";
}

void HttpServer::handlePUT(Response& res, const Request& req) {
    std::string path = "www" + req.path;

    std::ofstream f(path, std::ios::binary);
    f << req.body;
    f.close();

    res.statusCode = 201;
    res.statusText = "Created";
    res.body = "File saved to " + req.path;
}

void HttpServer::handleDELETE(Response& res, const Request& req) {
    std::string path = "www" + req.path;

    int status = std::remove(path.c_str());

    if (status == 0) {
        res.statusCode = 200;
        res.statusText = "OK";
        res.body = "Deleted " + req.path;
    } else {
        res.statusCode = 404;
        res.statusText = "Not Found";
        res.body = "File not found: " + req.path;
    }
}
// =======================
// handleClient
// =======================
void HttpServer::handleClient(SSL* ssl, int clientSocketFd, const Request& req) {
    std::cout << "[DEBUG] handleClient START, path=[" << req.path << "]\n";

    Response res;
    res.headers["Content-Type"] = "text/plain";
    res.headers["Connection"] = "close";

    bool handled = false;

    // 1) favicon
    if (req.path == "/favicon.ico") {
        res.statusCode = 404;
        res.statusText = "Not Found";
        res.body = "";
        handled = true;
    }

    // 2) Static file (GET only)
    if (!handled && req.method == "GET") {
        if (serveStaticFile(res, req.path)) {
            handled = true;
        }
    }

    // 3) Workload + router
    if (!handled) {
        // ===== SMART WORKLOAD ENGINE =====
        static double loadFactor = 20000.0;
        int w = std::max(1, (int)req.path.size());
        volatile long dummy = 0;
        long iterations = (long)(w * loadFactor);

        auto t0 = std::chrono::steady_clock::now();
        for (long i = 0; i < iterations; ++i) dummy += i;
        auto t1 = std::chrono::steady_clock::now();

        double busySec = std::chrono::duration<double>(t1 - t0).count();
        SystemMetrics::addBusy(busySec);

        double cpu = SystemMetrics::getCpuUsage();
        if (cpu < 30.0)
            loadFactor *= 1.10;
        else if (cpu > 85.0)
            loadFactor *= 0.85;

        loadFactor = std::clamp(loadFactor, 5000.0, 200000.0);

        // ===== ROUTER =====
        if (req.method == "GET") {
            handleGET(res, req);
        } else if (req.method == "POST") {
            handlePOST(res, req);
        } else if (req.method == "PUT") {
            handlePUT(res, req);
        } else if (req.method == "DELETE") {
            handleDELETE(res, req);
        } else {
            res.statusCode = 405;
            res.statusText = "Method Not Allowed";
            res.body = "Unsupported method";
        }
    }

    // 4) ALWAYS send response here (1 lần duy nhất)
    std::string raw = res.build();
    sendAllSSL(ssl, raw.c_str(), raw.size());
    int sd = SSL_shutdown(ssl);
    SSL_free(ssl);
    close(clientSocketFd);
}