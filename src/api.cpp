#include "api.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

CacheAPI::CacheAPI(std::shared_ptr<Cache> cache) : cache_(std::move(cache)) {}

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
    });

    // Start server
    server_.listen(host.c_str(), port);
}