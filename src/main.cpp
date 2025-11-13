#include "api.h"
#include "cache.h"
#include "replication.h"
#include "leader_elector.h"
#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    std::string role = "leader";
    int port = 5000;
    std::vector<std::string> followers;
    std::string self_url = "http://127.0.0.1:" + std::to_string(port);

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--role" && i + 1 < argc) role = argv[++i];
        else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
            self_url = "http://127.0.0.1:" + std::to_string(port);
        }
        else if (arg == "--followers" && i + 1 < argc) followers.push_back(argv[++i]);
    }

    auto cache = std::make_shared<Cache>(100, 100); // 100 ms
    ReplicationManager repl;

    // Manage API through unique_ptr so we can recreate if promoted
    std::unique_ptr<CacheAPI> api;

    if (role == "leader") {
        for (auto& f : followers) {
            repl.addFollower(f);
        }
        api = std::make_unique<CacheAPI>(cache, &repl);
    } else {
        api = std::make_unique<CacheAPI>(cache, nullptr);
    }

    // Leader elector (interval=2000ms, threshold=3)
    LeaderElector elector(
        self_url,
        {},  // no peer priorities for now
        role == "leader" ? self_url : "",
        2000,
        3,
        [&]() {
            std::cerr << "✅ Promoted to leader!" << std::endl;
            for (auto& f : followers) {
                repl.addFollower(f);
            }
            // Recreate API with replication enabled
            api = std::make_unique<CacheAPI>(cache, &repl);
        }
    );

    elector.start();

    api->start("0.0.0.0", port);

    elector.stop();
    return 0;
}