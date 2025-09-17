#include "leader.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

// Helper sleep wrapper
static void short_wait(int ms = 300) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Simple fake peers to simulate cluster state
TEST(LeaderElectionTest, BecomesLeaderIfNoPeers) {
    LeaderElector elector("node1", 7001, {}); 
    elector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(elector.isLeader());
    elector.stop();
}

TEST(LeaderElectionTest, NotLeaderIfPeerHasHigherId) {
    // Node2 should win because its ID is lexicographically larger
    LeaderElector elector("node1", 7001, {"node2"});
    elector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_FALSE(elector.isLeader());
    elector.stop();
}

TEST(LeaderElectionTest, LeadershipSwitchesOnStop) {
    LeaderElector elector("node1", 7001, {});
    elector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(elector.isLeader());

    elector.stop();
    EXPECT_FALSE(elector.isLeader());  // Once stopped, no longer leader
}

TEST(LeaderElectionTest, SingleNodeBecomesLeader) {
    LeaderElector node1("node1", 7001, {}); 
    node1.start();
    short_wait();
    EXPECT_TRUE(node1.isLeader());
    node1.stop();
}

TEST(LeaderElectionTest, ChoosesHighestPriorityLeader) {
    // node2 > node1 (lexicographically or ID priority)
    LeaderElector node1("node1", 7001, {"node2"});
    LeaderElector node2("node2", 7002, {"node1"});
    
    node1.start();
    node2.start();
    short_wait();

    EXPECT_FALSE(node1.isLeader());
    EXPECT_TRUE(node2.isLeader());

    node1.stop();
    node2.stop();
}

// ---------------------------------------------------------
// Failure & re-election scenarios
// ---------------------------------------------------------

TEST(LeaderElectionTest, LeaderFailureTriggersNewLeader) {
    LeaderElector node1("node1", 7001, {"node2"});
    LeaderElector node2("node2", 7002, {"node1"});
    
    node1.start();
    node2.start();
    short_wait();

    // Suppose node2 becomes leader
    ASSERT_TRUE(node2.isLeader());
    ASSERT_FALSE(node1.isLeader());

    // Simulate failure of node2
    node2.stop();
    short_wait(500); // give node1 some time to detect & promote

    EXPECT_TRUE(node1.isLeader()) << "Node1 should take over after Node2 fails";

    node1.stop();
}

TEST(LeaderElectionTest, LeaderResignationPromotesFollower) {
    LeaderElector leader("leader", 7101, {"follower"});
    LeaderElector follower("follower", 7102, {"leader"});

    leader.start();
    follower.start();
    short_wait();

    ASSERT_TRUE(leader.isLeader());
    ASSERT_FALSE(follower.isLeader());

    // Gracefully stop leader (simulate voluntary resignation)
    leader.stop();
    short_wait(500);

    EXPECT_TRUE(follower.isLeader()) << "Follower should become leader after resignation";

    follower.stop();
}
