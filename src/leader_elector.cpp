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

LeaderElector::~LeaderElector() {
    stop();
}

void LeaderElector::start() {
    if (running_.load()) return;
    running_.store(true);
    // 🚀 Promote self immediately if no peers
    if (peers_.empty()) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            leader_url_ = self_url_;
        }
        if (promote_cb_) promote_cb_();
        return; // No need to start loop
    }
    thread_ = std::thread([this]{ loop(); });
}


void LeaderElector::stop() {
    if (!running_.load()) return;
    running_.store(false);
    std::cerr << "[Elector] Stopping election loop for " << self_url_ << std::endl;
    if (thread_.joinable()) thread_.join();
    {
        std::lock_guard<std::mutex> lock(mtx_);
        leader_url_.clear();
    }
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
        cli.set_connection_timeout(std::chrono::milliseconds(300));
        cli.set_read_timeout(std::chrono::milliseconds(300));
        cli.set_write_timeout(std::chrono::milliseconds(300));
        auto res = cli.Get("/healthz");
        if (res && res->status == 200) return true;
    } catch (...) {}
    return false;
}

void LeaderElector::loop() {
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));

        if (leader_url_.empty()) {
            if (peers_.empty()) {
                set_leader(self_url_);
            if (promote_cb_) promote_cb_();
        } else {
            std::sort(peers_.begin(), peers_.end(),
                  [](auto &a, auto &b){ return a.second > b.second; });
            set_leader(peers_[0].first);
            }
        }

        // If no known leader, perform initial election (including self)
        if (leader_url_.empty()) {
            std::vector<std::pair<std::string,int>> candidates = peers_;
            candidates.push_back({self_url_, 0});
            std::sort(candidates.begin(), candidates.end(),
                      [](auto &a, auto &b){ return a.second > b.second; });
            for (auto &c : candidates) {
                if (poll_health(c.first)) {
                    std::lock_guard<std::mutex> lock(mtx_);
                    leader_url_ = c.first;
                    if (c.first == self_url_) {
                        std::cerr << "[Elector] Initial self-promotion: " << self_url_ << "\n";
                        promote_cb_();
                    }
                    break;
                }
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