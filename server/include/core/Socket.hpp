#pragma once
#include <string>

class Socket {
public:
    Socket();
    ~Socket();

    bool bind(int port);
    bool listen();
    int acceptClient();
    void closeSocket();

private:
    int serverFd;
};
