#pragma once
#ifndef REPLICATION_H
#define REPLICATION_H

#include <string>
#include <vector>
#include <cstdint>

class ReplicationManager {
public:
    ReplicationManager();

    // Add a follower node
    void addFollower(const std::string& address);
    
    // Forward a PUT request to all followers
    void replicatePut(const std::string& key, const std::string& value, uint64_t ttl);

    // Forward a DELETE request to all followers
    void replicateDelete(const std::string& key);

private:
    std::vector<std::string> followers_;
};

#endif // REPLICATION_H