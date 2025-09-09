#include <gtest/gtest.h>
#include "replication.h"

// Dummy test for now, just to wire things in.
// Later we can expand with mocks/fakes.
TEST(ReplicationTest, CanAddFollower) {
    ReplicationManager repl;
    repl.addFollower("http://127.0.0.1:5001");
    SUCCEED();  // If it compiles and runs, test passes
}

TEST(ReplicationTest, ReplicatePutDoesNotCrash) {
    ReplicationManager repl;
    repl.addFollower("http://127.0.0.1:5001");
    repl.replicatePut("foo", "bar", 60);
    SUCCEED();
}

TEST(ReplicationTest, ReplicateDeleteDoesNotCrash) {
    ReplicationManager repl;
    repl.addFollower("http://127.0.0.1:5001");
    repl.replicateDelete("foo");
    SUCCEED();
}