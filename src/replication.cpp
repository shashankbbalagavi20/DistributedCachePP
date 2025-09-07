#include "replication.h"
#include "httplib.h"
#include <iostream>

ReplicationManager::ReplicationManager() = default;

void ReplicationManager::addFollower(const std::string& address){
    followers_.push_back(address);
    std::cerr << "Added follower: " << address << std::endl;
}

void ReplicationManager::replicatePut(const std::string& key, const std::string& value, uint64_t ttl){
    for(const auto& follower: followers_){
        try {
            httplib::Client cli(follower.c_str());
            cli.set_read_timeout(2, 0); // 2 seconds timeout
            cli.set_write_timeout(2, 0);

            std::string body = "{\"value\":\"" + value + "\", \"ttl\":" + std::to_string(ttl) + "}";
            auto res = cli.Put(("/cache/" + key).c_str(), body, "application/json");

            if(res && res->status == 200){
                std::cerr << "Replicated PUT " << key << " -> " << follower << std::endl; 
            }
            else {
                std::cerr << "Failed PUT replication to " << follower << std::endl;
            }
        }
        catch(...){
            std::cerr << "Exception during PUT replication to " << follower << std::endl;
        }
    }
}

void ReplicationManager::replicateDelete(const std::string& key){
    for(const auto& follower: followers_){
        try{
            httplib::Client cli(follower.c_str());
            cli.set_read_timeout(2, 0); // 2 seconds timeout
            cli.set_write_timeout(2, 0);

            auto res = cli.Delete(("/cache/" + key).c_str());
            if(res && res->status == 200){
                std::cerr << "Replicated DELETE " << key << " -> " << follower << std::endl;
            }
            else {
                std::cerr << "Failed DELETE replication to " << follower << std::endl;
            }
        }
        catch(...){
            std::cerr << "Exception during DELETE replication to " << follower << std::endl;
        }
    }
}