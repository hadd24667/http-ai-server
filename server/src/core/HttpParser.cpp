#include "core/HttpParser.hpp"
#include <algorithm>
#include <sstream>

static inline void trimCRLF(std::string& s) {
    // remove trailing \r or \n
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) {
        s.pop_back();
    }
}

Request HttpParser::parse(const std::string& raw) {
    Request req;
    std::istringstream stream(raw);
    std::string line;

    // -------- Parse request line --------
    if (std::getline(stream, line)) {
        trimCRLF(line); // remove trailing \r

        std::istringstream firstLine(line);
        firstLine >> req.method >> req.path >> req.version;

        trimCRLF(req.method);
        trimCRLF(req.path);
        trimCRLF(req.version);
    }

    // -------- Parse headers --------
    while (std::getline(stream, line)) {
        trimCRLF(line);

        if (line.empty()) break; // end of headers (after \r\n)

        auto colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        // trim spaces
        while (!value.empty() && value.front() == ' ')
            value.erase(value.begin());

        trimCRLF(key);
        trimCRLF(value);

        req.headers[key] = value;
    }

    // -------- Parse body (if any) --------
    std::string body;
    while (std::getline(stream, line)) {
        trimCRLF(line);
        body += line;
        body.push_back('\n');
    }

    // remove last '\n'
    if (!body.empty() && body.back() == '\n')
        body.pop_back();

    req.body = body;

    return req;
}
