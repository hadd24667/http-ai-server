#pragma once
#include <iostream>
#include <string>
#include "utils/Config.hpp"

class HttpServer {
private:
    int port;
    int threads;
    std::string ai_url;

public:
    // Constructor mặc định
    HttpServer() = default;

    // Constructor nhận Config
    explicit HttpServer(const Config& cfg)
        : port(cfg.port), threads(cfg.threads), ai_url(cfg.ai_url) {}

    void start() {
        std::cout << "[Server] Starting HTTP server on port " << port
                  << " with " << threads << " threads." << std::endl;
        std::cout << "[Server] Connected to AI module: " << ai_url << std::endl;
        // Ở đây bạn sẽ thêm code Socket + ThreadPool sau
    }
};
