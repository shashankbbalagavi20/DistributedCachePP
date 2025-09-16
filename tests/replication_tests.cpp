#include <gtest/gtest.h>
#include <httplib.h>
#include "replication.h"
#include "cache.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// A tiny fake follower server for capturing requests
class FakeFollower {
public:
    FakeFollower(int port) : port_(port) {}

    void start() {
        server_.Put("/cache/(.*)", [&](const httplib::Request& req, httplib::Response& res) {
            lastPutKey = req.matches[1];
            lastPutBody = req.body;
            res.set_content(R"({"status":"ok"})", "application/json");
        });

        server_.Delete("/cache/(.*)", [&](const httplib::Request& req, httplib::Response& res) {
            lastDeleteKey = req.matches[1];
            res.set_content(R"({"status":"deleted"})", "application/json");
        });

        thread_ = std::thread([&]() {
            server_.listen("127.0.0.1", port_);
        });

        // Allow server time to bind
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void stop() {
        server_.stop();
        if (thread_.joinable()) thread_.join();
    }

    std::string lastPutKey;
    std::string lastPutBody;
    std::string lastDeleteKey;

private:
    httplib::Server server_;
    int port_;
    std::thread thread_;
};

TEST(ReplicationTest, ReplicatesPutToFollower) {
    FakeFollower follower(6001);
    follower.start();

    ReplicationManager repl;
    repl.addFollower("http://127.0.0.1:6001");

    repl.replicatePut("foo", "bar", 42);

    follower.stop();

    // Assertions
    EXPECT_EQ(follower.lastPutKey, "foo");
    EXPECT_NE(follower.lastPutBody.find("bar"), std::string::npos);
}

TEST(ReplicationTest, ReplicatesDeleteToFollower) {
    FakeFollower follower(6002);
    follower.start();

    ReplicationManager repl;
    repl.addFollower("http://127.0.0.1:6002");

    repl.replicateDelete("foo");

    follower.stop();

    // Assertions
    EXPECT_EQ(follower.lastDeleteKey, "foo");
}

TEST(ReplicationTest, HandlesUnreachableFollowerGracefully) {
    // Do not start follower (simulate unreachable node)
    ReplicationManager repl;
    repl.addFollower("http://127.0.0.1:6999"); // Port not in use

    // Attempt to replicate
    EXPECT_NO_THROW({
        repl.replicatePut("foo", "bar", 60);
        repl.replicateDelete("foo");
    });

    // Since the follower was unreachable, nothing should crash,
    // and the system should continue running.
}

TEST(ReplicationTest, StressReplicationToFollowers) {
    // Start two follower cache instances
    Cache follower1(1000);
    Cache follower2(1000);

    httplib::Server server1;
    httplib::Server server2;

    // Setup follower1 endpoints
    server1.Put(R"(/cache/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body = nlohmann::json::parse(req.body);
        follower1.put(req.matches[1], body["value"], body["ttl"]);
        res.set_content(R"({"status":"ok"})", "application/json");
    });
    server1.Delete(R"(/cache/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        follower1.erase(req.matches[1]);
        res.set_content(R"({"status":"deleted"})", "application/json");
    });

    // Setup follower2 endpoints
    server2.Put(R"(/cache/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body = nlohmann::json::parse(req.body);
        follower2.put(req.matches[1], body["value"], body["ttl"]);
        res.set_content(R"({"status":"ok"})", "application/json");
    });
    server2.Delete(R"(/cache/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        follower2.erase(req.matches[1]);
        res.set_content(R"({"status":"deleted"})", "application/json");
    });

    // Run followers in background
    std::thread t1([&]() { server1.listen("127.0.0.1", 7101); });
    std::thread t2([&]() { server2.listen("127.0.0.1", 7102); });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ReplicationManager repl;
    repl.addFollower("http://127.0.0.1:7101");
    repl.addFollower("http://127.0.0.1:7102");

    // Stress test: replicate 100 keys
    for (int i = 0; i < 100; i++) {
        std::string key = "key" + std::to_string(i);
        std::string val = "val" + std::to_string(i);
        repl.replicatePut(key, val, 60);
    }

    // Verify last key replicated correctly on both followers
    EXPECT_EQ(follower1.get("key99").value_or(""), "val99");
    EXPECT_EQ(follower2.get("key99").value_or(""), "val99");

    // cleanup
    server1.stop();
    server2.stop();
    t1.join();
    t2.join();
}
