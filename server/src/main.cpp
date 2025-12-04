#include "core/HttpServer.hpp"
#include "utils/Config.hpp"
#include "monitor/SystemMetrics.hpp"
#include <signal.h>
#include <iostream>

int main() {
    signal(SIGPIPE, SIG_IGN);
    SystemMetrics::init();

    Config cfg("config/server.json");

    HttpServer server(cfg.port, cfg.threads); //http://172.27.176.157:8080
    server.start();

    return 0;
}