#include "core/Response.hpp"

std::string Response::build() const {
    std::string res;

    res += "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n";

    // Luôn tự set Content-Length
    res += "Content-Length: " + std::to_string(body.size()) + "\r\n";

    // Thêm tất cả header
    for (const auto& h : headers) {
        res += h.first + ": " + h.second + "\r\n";
    }

    res += "\r\n";
    res += body;

    return res;
}

