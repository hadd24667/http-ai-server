#include "ai/AIClient.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::string* s = static_cast<std::string*>(userp);
    s->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

AIClient::AIClient(std::string url, long timeoutMs)
    : url_(std::move(url)), timeoutMs_(timeoutMs) {}

std::optional<std::string> AIClient::predict(const AIFeatures& f) const {
    CURL* curl = curl_easy_init();
    if (!curl) return std::nullopt;

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    json payload = {
        {"cpu", f.cpu},
        {"queue_len", f.queue_len},
        {"queue_bin", f.queue_bin},
        {"request_method", f.request_method},
        {"request_path_length", f.request_path_length},
        {"estimated_workload", f.estimated_workload},
        {"req_size", f.req_size}
    };

    std::string body = payload.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs_);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeoutMs_);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || httpCode < 200 || httpCode >= 300) return std::nullopt;

    try {
        auto j = json::parse(response);
        if (!j.contains("algorithm")) return std::nullopt;
        return j["algorithm"].get<std::string>();
    } catch (...) {
        return std::nullopt;
    }
}
