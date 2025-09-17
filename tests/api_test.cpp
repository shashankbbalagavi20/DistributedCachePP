#include <gtest/gtest.h>
#include <thread>
#include <nlohmann/json.hpp>
#include <chrono>
#include "../include/cache.h"
#include "../include/api.h"
#include <httplib.h>

using json = nlohmann::json;

TEST(ApiTest, BasicCRUD){
    auto cache = std::make_shared<Cache>(10);
    CacheAPI api(cache);

    // Run server in background thread
    std::thread server_thread([&api]() {
        api.start("127.0.0.1", 5001);
    });

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    httplib::Client cli("127.0.0.1", 5001);

    // PUT
    auto put_res = cli.Put("/cache/foo", "{\"value\":\"bar\",\"ttl\":500}", "application/json");
    ASSERT_TRUE(put_res != nullptr);
    EXPECT_EQ(put_res->status, 200);

    json put_json = json::parse(put_res->body);
    EXPECT_EQ(put_json["status"], "ok");

    // GET
    auto get_res = cli.Get("/cache/foo");
    ASSERT_TRUE(get_res != nullptr);
    EXPECT_EQ(get_res->status, 200);

    json get_json = json::parse(get_res->body);
    EXPECT_EQ(get_json["value"], "bar");

    // DELETE
    auto del_res = cli.Delete("/cache/foo");
    ASSERT_TRUE(del_res != nullptr);

    json del_json = json::parse(del_res->body);
    EXPECT_EQ(del_json["status"], "deleted");

    // GET again (should be 404)
    auto get_res2 = cli.Get("/cache/foo");
    ASSERT_TRUE(get_res2 != nullptr);

    json get2_json = json::parse(get_res2->body);
    EXPECT_EQ(get2_json["error"], "not found");

    // Check metrics
    auto metrics_res = cli.Get("/metrics");
    ASSERT_TRUE(metrics_res != nullptr);
    EXPECT_EQ(metrics_res->status, 200);
    EXPECT_NE(metrics_res->body.find("cache_hits_total"), std::string::npos);

    // Clean shutdown
    api.stop();
    server_thread.join();
}

TEST(ApiTest, InvalidPut) {
    auto cache = std::make_shared<Cache>(10);
    CacheAPI api(cache);
    std::thread server_thread([&api]() { api.start("127.0.0.1", 5002); });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    httplib::Client cli("127.0.0.1", 5002);
    auto res = cli.Put("/cache/foo", "not-json", "application/json");

    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 400);

    api.stop();
    server_thread.join();
}

TEST(ApiTest, HealthzEndpointRespondsOk) {
    // Setup cache + API
    auto cache = std::make_shared<Cache>(10, std::chrono::milliseconds(1000));
    ReplicationManager repl;
    CacheAPI api(cache, &repl);

    // Start API on a test port
    int port = 8085; // Use a non-conflicting port
    std::thread server_thread([&]() {
        api.start("127.0.0.1", port);
    });

    // Give server time to bind
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Send request to /healthz
    httplib::Client cli("127.0.0.1", port);
    auto res = cli.Get("/healthz");

    // Validate response
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_NE(res->body.find("\"status\":\"ok\""), std::string::npos);

    // Shutdown
    api.stop();
    if (server_thread.joinable()) server_thread.join();
}