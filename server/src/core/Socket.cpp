#include "core/Socket.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <netinet/tcp.h>   // TCP_NODELAY

Socket::Socket() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "[ERROR] Cannot create socket\n";
        return;
    }

    int opt = 1;

    // Cho phép bind lại nhanh
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "[WARN] setsockopt SO_REUSEADDR failed\n";
    }

    // Cho phép nhiều process / thread bind cùng port (tương lai nếu cần)
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        std::cerr << "[WARN] setsockopt SO_REUSEPORT failed\n";
    }

    // Tắt Nagle cho server socket (giảm độ trễ)
    if (setsockopt(serverFd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
        std::cerr << "[WARN] setsockopt TCP_NODELAY failed\n";
    }
}

Socket::~Socket() {
    if (serverFd >= 0) ::close(serverFd);
}

bool Socket::bind(int port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    return ::bind(serverFd, (sockaddr*)&addr, sizeof(addr)) >= 0;
}

bool Socket::listen() {
    // tăng backlog để wrk mở nhiều connection không bị drop
    return ::listen(serverFd, 1024) >= 0;
}

int Socket::acceptClient() {
    return ::accept(serverFd, nullptr, nullptr);
}

void Socket::closeSocket() {
    if (serverFd >= 0) {
        ::close(serverFd);
        serverFd = -1;
    }
}
