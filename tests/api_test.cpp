#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../include/cache.h"
#include "../include/api.h"
#include <httplib.h>

TEST(ApiTest, BasicCRUD){
    auto cache = std::make_shared<Cache>(10);
    CacheAPI api(cache);

    // Run server in background thread
    std::thread server_thread([&api]() {
        api.start("127.0.0.1", 5001);
    });

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    httplib::Client cli("127.0.0.1", 5001);

    // PUT
    auto put_res = cli.Put("/cache/foo", "{\"value\":\"bar\",\"ttl\":500}", "application/json");
    ASSERT_TRUE(put_res != nullptr);
    EXPECT_EQ(put_res->status, 200);
    EXPECT_NE(put_res->body.find("\"status\":\"ok\""), std::string::npos);

    // GET
    auto get_res = cli.Get("/cache/foo");
    ASSERT_TRUE(get_res != nullptr);
    EXPECT_EQ(get_res->status, 200);
    EXPECT_NE(get_res->body.find("\"bar\""), std::string::npos);

    // DELETE
    auto del_res = cli.Delete("/cache/foo");
    ASSERT_TRUE(del_res != nullptr);
    EXPECT_EQ(del_res->status, 200);

    // GET again (should be 404)
    auto get_res2 = cli.Get("/cache/foo");
    ASSERT_TRUE(get_res2 != nullptr);
    EXPECT_EQ(get_res2->status, 404);

    // Check metrics
    auto metrics_res = cli.Get("/metrics");
    ASSERT_TRUE(metrics_res != nullptr);
    EXPECT_EQ(metrics_res->status, 200);
    EXPECT_NE(metrics_res->body.find("cache_hits_total"), std::string::npos);

    // Cleanup
    // Stop server by terminating the process
    std::terminate(); // For now: abrupt exit to stop blocking server thread
}