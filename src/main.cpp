#include "api.h"
#include "cache.h"
#include "replication.h"
#include "leader.h"
#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
    std::string role = "follower";   // default: follower
    int port = 5000;
    int electionPort = 7000;
    std::string nodeId = "node1";
    std::vector<std::string> peers;
    std::vector<std::string> followers;

    // --- Parse args ---
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--role" && i + 1 < argc) role = argv[++i];
        else if (arg == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
        else if (arg == "--id" && i + 1 < argc) nodeId = argv[++i];
        else if (arg == "--election-port" && i + 1 < argc) electionPort = std::stoi(argv[++i]);
        else if (arg == "--peers" && i + 1 < argc) peers.push_back(argv[++i]);
        else if (arg == "--followers" && i + 1 < argc) followers.push_back(argv[++i]);
    }

    // --- Core components ---
    auto cache = std::make_shared<Cache>(100);
    ReplicationManager repl;
    CacheAPI api(cache, nullptr); // attach repl later if we’re leader

    // --- Leader election setup ---
    LeaderElector elector(nodeId, electionPort, peers);
    std::thread electionThread([&]() {
        elector.start();  // runs in background
    });

    // --- Dynamic role switch ---
    std::thread roleThread([&]() {
        while (true) {
            if (elector.isLeader()) {
                if (!api.hasReplication()) {
                    std::cerr << "⚡ Became LEADER, enabling replication\n";
                    api.setReplicationManager(&repl);
                    for (auto& f : followers) {
                        repl.addFollower(f);
                    }
                }
            } else {
                if (api.hasReplication()) {
                    std::cerr << "📥 Became FOLLOWER, disabling replication\n";
                    api.setReplicationManager(nullptr);
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    });

    // --- Start REST API ---
    api.start("0.0.0.0", port);

    // --- Cleanup ---
    api.stop();
    elector.stop();
    if (electionThread.joinable()) electionThread.join();
    if (roleThread.joinable()) roleThread.join();

    return 0;
}
