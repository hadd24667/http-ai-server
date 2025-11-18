#include "core/HttpServer.hpp"
#include "utils/Config.hpp"
#include <iostream>

int main() {
    Config cfg("config/server.json");

    HttpServer server(cfg.port, cfg.threads);
    server.start();

    return 0;
}