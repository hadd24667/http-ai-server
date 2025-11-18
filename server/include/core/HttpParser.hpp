#pragma once
#include "core/Request.hpp"
#include <string>

class HttpParser {
public:
    static Request parse(const std::string& raw);
};
