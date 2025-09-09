#include "api.h"
#include "cache.h"
#include "replication.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string role = "leader";
    int port = 5000;
    std::vector<std::string> followers;

    for(int i=1; i<argc; i++){
        std::string arg = argv[i];
        if(arg == "--role" && i + 1 < argc) role = argv[++i];
        else if(arg == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
        else if(arg == "--followers" && i + 1 < argc) followers.push_back(argv[++i]);
    }

    auto cache = std::make_shared<Cache>(100);

    ReplicationManager repl;
    CacheAPI api(cache, role == "leader" ? &repl : nullptr);

    if(role == "leader"){
        for (auto& f : followers){
            repl.addFollower(f);
        }
    }

    api.start("0.0.0.0", port);
    return 0;
}