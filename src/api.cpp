#include "api.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <ctime>
#include <time_utils.h>

using json = nlohmann::json;

CacheAPI::CacheAPI(std::shared_ptr<Cache> cache) : cache_(std::move(cache)) {}

void CacheAPI::logRequest(const std::string& method, const std::string& path, int status)
{
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm_buf = safe_localtime(now);
    std::cerr << "[" << std::put_time(&tm_buf, "%F %T") << "] "
    << method << " " << path << " -> " << status << std::endl;
}

static std::string make_prometheus_metrics(const Cache &cache)
{
    std::ostringstream ss;
    ss << "# HELP cache_hits_total Total number of cache hits\n";
    ss << "# TYPE cache_hits_total counter\n";
    ss << "cache_hits_total " << cache.hits() << "\n\n";

    ss << "# HELP cache_misses_total Total number of cache misses\n";
    ss << "# TYPE cache_misses_total counter\n";
    ss << "cache_misses_total " << cache.misses() << "\n\n";

    ss << "# HELP cache_size Number of items currently stored in cache\n";
    ss << "# TYPE cache_size gauge\n";
    ss << "cache_size " << cache.size() << "\n\n";

    ss << "# HELP cache_capacity Configured cache capacity\n";
    ss << "# TYPE cache_capacity gauge\n";
    ss << "cache_capacity " << cache.capacity() << "\n\n";

    ss << "# HELP cache_eviction_interval_ms Async eviction interval in ms\n";
    ss << "# TYPE cache_eviction_interval_ms gauge\n";
    ss << "cache_eviction_interval_ms " << cache.eviction_interval() << "\n";

    return ss.str();
}

void CacheAPI::start(const std::string& host, int port){
    // GET /cache/<key>
    server_.Get(R"(/cache/(\w+))", [this](const httplib::Request& req, httplib::Response& res) {
        auto key = req.matches[1];
        auto val = cache_->get(key);
        if(val.has_value()){
            json j = {{"value", val.value()}};
            res.set_content(j.dump(), "application/json");
        } else {
            res.status = 404; // Not Found
            res.set_content(R"({"error": "not found"})", "application/json");
        }
        logRequest("GET", req.path, res.status);
    });

    // PUT /cache/<key>
    server_.Put(R"(/cache/(\w+))", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::cerr << "Raw request body: " << req.body << std::endl; // DEBUG

            auto key = req.matches[1];
            auto body_json = json::parse(req.body);

            if (!body_json.contains("value")) {
                res.status = 400;
                res.set_content(R"({"error": "missing 'value'"})", "application/json");
                return;
            }

            std::string value = body_json["value"];
            uint64_t ttl = body_json.value("ttl", 0);

            cache_->put(key, value, ttl);
            res.set_content(R"({"status": "ok"})", "application/json");
        }
        catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string{"{\"error\": \""} + e.what() + "\"}", "application/json");
        }
        logRequest("PUT", req.path, res.status);
    });

    // DELETE /cache/<key>
    server_.Delete(R"(/cache/(\w+))", [this](const httplib::Request& req, httplib::Response& res) {
        auto key = req.matches[1];
        if(cache_->erase(key)){
            res.set_content(R"({"status": "deleted"})", "application/json");
        }
        else{
            res.status = 404;
            res.set_content(R"({"error": "not found"})", "application/json");
        }
        logRequest("DELETE", req.path, res.status);
    });

    // METRICS endpoint (Prometheus plain text)
    server_.Get("/metrics", [this](const httplib::Request& req, httplib::Response& res) {
        auto body = make_prometheus_metrics(*cache_);
        res.set_content(body, "text/plain; version=0.0.4; charset=utf-8");
        res.status = 200;
        logRequest("GET", req.path, res.status);
    });

    // Start server
    std::cerr << "🚀 Starting REST API on " << host << ":" << port << std::endl;
    server_.listen(host.c_str(), port);
}