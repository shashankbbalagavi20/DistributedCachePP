#ifndef LEADER_ELECTOR_H
#define LEADER_ELECTOR_H

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include <chrono>
#include <mutex>

class LeaderElector {
public:
    using PromoteCallback = std::function<void()>;

    LeaderElector(std::string self_url,
                  std::vector<std::pair<std::string,int>> peers_with_priority,
                  std::string current_leader,
                  uint64_t interval_ms,
                  int failure_threshold,
                  PromoteCallback promote_cb);

    ~LeaderElector();

    void start();
    void stop();

    void set_leader(const std::string& leader_url);

    // 🔑 For tests / observability
    std::string get_current_leader();

private:
    void loop();
    bool poll_health(const std::string& url);

    std::string self_url_;
    std::vector<std::pair<std::string,int>> peers_;
    std::string leader_url_;

    uint64_t interval_ms_;
    int fail_threshold_;

    std::atomic<bool> running_;
    std::thread thread_;
    PromoteCallback promote_cb_;

    int consecutive_failures_;
    mutable std::mutex mtx_;
};

#endif // LEADER_ELECTOR_H