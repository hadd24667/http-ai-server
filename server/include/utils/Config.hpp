#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

class Config {
public:
    int port;
    int threads;
    std::string ai_url;
    std::string mode;

    Config(const std::string& path) {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open config file: " + path);
            }

            nlohmann::json j;
            file >> j;

            port    = j.value("port", 8080);
            threads = j.value("threads", 4);
            ai_url  = j.value("ai_url", "http://127.0.0.1:5000/predict");
            mode    = j.value("mode", "prod");   // ⭐ DEFAULT = prod

            // Normalize (đưa về lowercase)
            for (auto& c : mode) c = std::tolower(c);

            if (mode != "train" && mode != "prod") {
                std::cerr << "[Config] Invalid mode: " << mode 
                          << " — fallback to 'prod'\n";
                mode = "prod";
            }

            std::cout << "[Config] Loaded: port=" << port 
                      << ", threads=" << threads
                      << ", mode=" << mode 
                      << "\n";

        } catch (const std::exception& e) {
            std::cerr << "[Config] Error: " << e.what() << std::endl;

            // fallback DEFAULT values
            port = 8080;
            threads = 4;
            ai_url = "http://127.0.0.1:5000/predict";
            mode = "prod";
        }
    }
};
