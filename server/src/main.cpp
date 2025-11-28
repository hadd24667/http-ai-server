#include "core/HttpServer.hpp"
#include "utils/Config.hpp"
#include <iostream>

int main() {
    Config cfg("config/server.json");

    HttpServer server(cfg.port, cfg.threads); //http://172.27.176.157:8080
    server.start();

    return 0;
}