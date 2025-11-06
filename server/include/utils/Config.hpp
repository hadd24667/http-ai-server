#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp> // thư viện JSON (bạn sẽ thêm sau)

class Config {
public:
    int port;
    int threads;
    std::string ai_url;

    Config(const std::string& path) {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open config file: " + path);
            }

            nlohmann::json j;
            file >> j;

            port = j.value("port", 8080);
            threads = j.value("threads", 4);
            ai_url = j.value("ai_url", "http://127.0.0.1:5000/predict");
        } catch (const std::exception& e) {
            std::cerr << "[Config] Error: " << e.what() << std::endl;
        }
    }
};
