#include "core/HttpServer.hpp"
#include "utils/Config.hpp"

int main() {
    Config cfg("config/server.json");
    HttpServer server(cfg);
    server.start();
    return 0;
}
