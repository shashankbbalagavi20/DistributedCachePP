#include "leader_elector.h"
#include <httplib.h>
#include <algorithm>
#include <iostream>

LeaderElector::LeaderElector(std::string self_url,
                             std::vector<std::pair<std::string,int>> peers_with_priority,
                             std::string current_leader,
                             uint64_t interval_ms,
                             int failure_threshold,
                             PromoteCallback promote_cb)
    : self_url_(std::move(self_url)),
      peers_(std::move(peers_with_priority)),
      leader_url_(std::move(current_leader)),
      interval_ms_(interval_ms),
      fail_threshold_(failure_threshold),
      running_(false),
      promote_cb_(std::move(promote_cb)),
      consecutive_failures_(0)
{}

LeaderElector(std::string self_url,
              std::vector<std::pair<std::string,int>> peers = {},
              PromoteCallback promote_cb = [](){})
    : LeaderElector(std::move(self_url),
                    std::move(peers),
                    "",              // no current leader
                    500,             // default 500ms interval
                    3,               // default failure threshold
                    std::move(promote_cb)) 
{
}

LeaderElector::~LeaderElector() {
    stop();
}

void LeaderElector::start() {
    if (running_.load()) return;
    running_.store(true);
    thread_ = std::thread([this]{ loop(); });
}

void LeaderElector::stop() {
    if (!running_.load()) return;
    running_.store(false);
    if (thread_.joinable()) thread_.join();
}

void LeaderElector::set_leader(const std::string& leader_url) {
    std::lock_guard<std::mutex> lock(mtx_);
    leader_url_ = leader_url;
    consecutive_failures_ = 0;
}

std::string LeaderElector::get_current_leader() {
    std::lock_guard<std::mutex> lock(mtx_);
    return leader_url_;
}

bool LeaderElector::poll_health(const std::string& url) {
    try {
        httplib::Client cli(url.c_str());
        cli.set_connection_timeout(std::chrono::seconds(1));
        auto res = cli.Get("/healthz");
        if (res && res->status == 200) return true;
    } catch (...) {}
    return false;
}

bool isLeader() {
    return leader_url_ == self_url_;
}

void LeaderElector::loop() {
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));

        if (leader_url_.empty()) {
            // pick by priority
            std::sort(peers_.begin(), peers_.end(),
                      [](auto &a, auto &b){ return a.second > b.second; });
            if (!peers_.empty()) {
                std::lock_guard<std::mutex> lock(mtx_);
                leader_url_ = peers_[0].first;
            }
        }

        std::string current_leader;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            current_leader = leader_url_;
        }

        if (poll_health(current_leader)) {
            consecutive_failures_ = 0;
            continue;
        }

        consecutive_failures_++;
        if (consecutive_failures_ < fail_threshold_) continue;

        // Election: check peers + self
        std::vector<std::pair<std::string,int>> candidates = peers_;
        candidates.push_back({self_url_, 0}); // self gets default lowest priority unless set by user
        std::sort(candidates.begin(), candidates.end(),
                  [](auto &a, auto &b){ return a.second > b.second; });

        for (auto &c : candidates) {
            if (poll_health(c.first)) {
                if (c.first == self_url_) {
                    std::cerr << "[Elector] Promoting self (" << self_url_ << ") to leader\n";
                    promote_cb_();
                }
                {
                    std::lock_guard<std::mutex> lock(mtx_);
                    leader_url_ = c.first;
                }
                consecutive_failures_ = 0;
                break;
            }
        }
    }
}