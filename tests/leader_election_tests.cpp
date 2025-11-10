#include "leader_elector.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

// Helper sleep wrapper
static void short_wait(int ms = 300) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Simple fake peers to simulate cluster state
TEST(LeaderElectionTest, BecomesLeaderIfNoPeers) {
    LeaderElector elector(
    "node1",
    {},               // no peers
    "",               // current leader
    500,              // interval
    3,                // failure threshold
    {}                // callback
    ); 
    elector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(elector.get_current_leader(), "node1");
    elector.stop();
}

TEST(LeaderElectionTest, NotLeaderIfPeerHasHigherId) {
    // Node2 should win because its ID is lexicographically larger
    LeaderElector elector(
    "node1",
    {{"node2", 1}},   // peers with priorities
    "",               // current leader
    500,
    3,
    {}
   );
    elector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_NE(elector.get_current_leader(), "node1");
    elector.stop();
}

TEST(LeaderElectionTest, LeadershipSwitchesOnStop) {
    LeaderElector elector(
    "node1",
    {},               // no peers
    "",               // current leader
    500,              // interval
    3,                // failure threshold
    {}                // callback
    ); 
    elector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_EQ(elector.get_current_leader(), "node1");

    elector.stop();
    EXPECT_NE(elector.get_current_leader(), "node1");  // Once stopped, no longer leader
}

TEST(LeaderElectionTest, SingleNodeBecomesLeader) {
    LeaderElector elector(
    "node1",
    {},               // no peers
    "",               // current leader
    500,              // interval
    3,                // failure threshold
    {}                // callback
    ); 
    elector.start();
    short_wait();
    EXPECT_EQ(elector.get_current_leader(), "node1");
    elector.stop();
}

TEST(LeaderElectionTest, ChoosesHighestPriorityLeader) {
    // node2 > node1 (lexicographically or ID priority)
    LeaderElector node1(
    "node1",
    {{"node2", 1}},   // peers with priorities
    "",               // current leader
    500,
    3,
    {}
   );
    LeaderElector node2(
    "node2",
    {{"node1", 1}},   // peers with priorities
    "",               // current leader
    500,
    3,
    {}
   );
    node1.start();
    node2.start();
    short_wait();

    EXPECT_NE(node1.get_current_leader(), "node1");
    EXPECT_EQ(node2.get_current_leader(), "node2");

    node1.stop();
    node2.stop();
}

// ---------------------------------------------------------
// Failure & re-election scenarios
// ---------------------------------------------------------

TEST(LeaderElectionTest, LeaderFailureTriggersNewLeader) {
    LeaderElector node1(
    "node1",
    {{"node2", 1}},   // peers with priorities
    "",               // current leader
    500,
    3,
    {}
   );
    LeaderElector node2(
    "node2",
    {{"node1", 1}},   // peers with priorities
    "",               // current leader
    500,
    3,
    {}
   );
    
    node1.start();
    node2.start();
    short_wait();

    // Suppose node2 becomes leader
    ASSERT_EQ(node2.get_current_leader(), "node2");
    ASSERT_NE(node1.get_current_leader(), "node1");

    // Simulate failure of node2
    node2.stop();
    short_wait(500); // give node1 some time to detect & promote

    EXPECT_EQ(node1.get_current_leader(), "node1") << "Node1 should take over after Node2 fails";

    node1.stop();
}

TEST(LeaderElectionTest, LeaderResignationPromotesFollower) {
    LeaderElector leader(
    "leader",
    {{"follower", 1}},   // peers with priorities
    "",               // current leader
    500,
    3,
    {}
   );
    LeaderElector follower(
    "follower",
    {{"leader", 1}},   // peers with priorities
    "",               // current leader
    500,
    3,
    {}
   );

    leader.start();
    follower.start();
    short_wait();

    ASSERT_EQ(leader.get_current_leader(), "leader");
    ASSERT_NE(follower.get_current_leader(), "follower");

    // Gracefully stop leader (simulate voluntary resignation)
    leader.stop();
    short_wait(500);

    EXPECT_EQ(follower.get_current_leader(), "follower") << "Follower should become leader after resignation";

    follower.stop();
}

class FakeHealthServer {
public:
    FakeHealthServer(int port) : port_(port), running_(false) {}

    void start() {
        server_.Get("/healthz", [&](const httplib::Request&, httplib::Response& res) {
            res.status = 200;
            res.set_content("ok", "text/plain");
        });

        running_ = true;
        thread_ = std::thread([&]() {
            server_.listen("127.0.0.1", port_);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // allow bind
    }

    void stop() {
        if (running_) {
            server_.stop();
            running_ = false;
            if (thread_.joinable()) thread_.join();
        }
    }

private:
    int port_;
    httplib::Server server_;
    std::thread thread_;
    bool running_;
};

// --- Tests ---
TEST(LeaderElectorTest, PromotesSelfWhenLeaderFails) {
    std::atomic<bool> promoted{false};

    // Start a fake leader on 5001
    FakeHealthServer leader(5001);
    leader.start();

    LeaderElector elector(
        "http://127.0.0.1:5002",                   // self
        {{"http://127.0.0.1:5001", 10}},           // peers with priority
        "http://127.0.0.1:5001",                   // current leader
        200,                                       // check interval (ms)
        2,                                         // failure threshold
        [&]() { promoted.store(true); }            // promote callback
    );

    elector.start();

    // Kill leader after short delay
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    leader.stop();

    // Wait enough for elector to detect failure & promote
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    elector.stop();

    EXPECT_TRUE(promoted.load()) << "Node should promote itself after leader fails";
}

TEST(LeaderElectorTest, DoesNotPromoteIfLeaderHealthy) {
    std::atomic<bool> promoted{false};

    // Fake healthy leader
    FakeHealthServer leader(5003);
    leader.start();

    LeaderElector elector(
        "http://127.0.0.1:5004",                   // self
        {{"http://127.0.0.1:5003", 10}},           // peers
        "http://127.0.0.1:5003",                   // current leader
        200,                                       // check interval
        2,                                         // failure threshold
        [&]() { promoted.store(true); }            // promote callback
    );

    elector.start();

    // Keep leader alive
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    elector.stop();
    leader.stop();

    EXPECT_FALSE(promoted.load()) << "Node should not promote while leader is healthy";
}

TEST(LeaderElectorTest, ElectsHighestPriorityHealthyPeer) {
    std::atomic<bool> promoted{false};

    // Start two peers (follower candidates)
    FakeHealthServer peer_low(5005);   // lower priority
    FakeHealthServer peer_high(5006);  // higher priority
    peer_low.start();
    peer_high.start();

    // Start elector with both peers
    LeaderElector elector(
        "http://127.0.0.1:5007", // self
        {
            {"http://127.0.0.1:5005", 5},  // low priority
            {"http://127.0.0.1:5006", 10}  // high priority
        },
        "http://127.0.0.1:5005",           // initial leader (low priority)
        200,                               // interval
        2,                                 // failure threshold
        [&]() { promoted.store(true); }    // self promote callback
    );

    elector.start();

    // Kill the low-priority leader
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    peer_low.stop();

    // Wait for elector to detect and re-elect
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    elector.stop();
    peer_high.stop();

    // We should *not* promote self (5007), but instead pick high-priority peer (5006)
    EXPECT_FALSE(promoted.load()) << "Self should not be promoted when higher-priority peer is healthy";
}