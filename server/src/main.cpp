#include "core/HttpServer.hpp"
#include "utils/Config.hpp"
#include "monitor/SystemMetrics.hpp"
#include <signal.h>
#include <iostream>

int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);
    SystemMetrics::init();

    // Default algorithm = ADAPTIVE
    std::string algo = "ADAPTIVE";

    // Read CLI args: --algo=X
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.rfind("--algo=", 0) == 0) {
            algo = arg.substr(7);
        }
    }

    std::cout << "[MAIN] Scheduling algorithm = " << algo << "\n";

    Config cfg("config/server.json");

    HttpServer server(cfg.port, cfg.threads, algo);
    server.start();

    return 0;
}

//cmake -S . -B build
//cmake --build build
//https://172.27.176.157:8080
//./build/server/http_server