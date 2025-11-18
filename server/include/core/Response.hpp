#pragma once
#include <string>
#include <unordered_map>

class Response {
public:
    int statusCode = 200;
    std::string statusText = "OK";

    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::string build() const;
};
