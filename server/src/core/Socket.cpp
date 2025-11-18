#include "core/Socket.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

Socket::Socket() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "[ERROR] Cannot create socket\n";
    }
}

Socket::~Socket() {
    if (serverFd >= 0) ::close(serverFd);
}

bool Socket::bind(int port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    return ::bind(serverFd, (sockaddr*)&addr, sizeof(addr)) >= 0;
}

bool Socket::listen() {
    return ::listen(serverFd, 128) >= 0;
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
