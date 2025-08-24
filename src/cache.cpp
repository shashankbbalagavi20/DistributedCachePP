#include "cache.h"
#include <mutex>
#include <shared_mutex>
#include "algorithm"

Cache::Cache(size_t capacity, uint64_t eviction_interval_ms) : 
        capacity_(capacity), eviction_interval_ms_(eviction_interval_ms)
{
    // Start async eviction thread
    eviction_thread_ = std::thread([this, eviction_interval_ms]() {
        eviction_loop(eviction_interval_ms);
    });
}

Cache::~Cache(){
    // Signal stop and join background thread
    stop_eviction_.store(true);
    if(eviction_thread_.joinable()){
        eviction_thread_.join();
    }
}

// PRECONDITION: caller holds mutex_ with a unique_lock
void Cache::evict_if_needed(){
    if(map_.size() <= capacity_){
        return;   // No eviction needed
    }

    // Evict from the back of the LRU list
    auto lru_key = lru_list_.back();
    lru_list_.pop_back();
    map_.erase(lru_key); // Remove from map
}

void Cache::put(const std::string& key, const std::string& value, uint64_t ttl_ms){
    auto now = clock::now();
    clock::time_point expiry_time;

    if(ttl_ms > 0){
        expiry_time = now + std::chrono::milliseconds(ttl_ms);
    }
    else{
        expiry_time = clock::time_point::max(); // Put expiry far in the future
    }

    std::unique_lock<std::shared_mutex> lock(mutex_);

    // Check if key already exists
    auto it = map_.find(key);
    if (it != map_.end()) {
        // If the existing record is expired, remove then fall through to fresh insert
        if (it->second.expiry != clock::time_point::max() && it->second.expiry < now) {
            lru_list_.erase(it->second.lru_it);
            map_.erase(it);
        } else {
            // Update existing
            it->second.value = value;
            it->second.expiry = expiry_time;
            touch_to_front(it);
            return;
        }
    }

    // Insert new key at front of LRU list
    lru_list_.push_front(key);

    // Add entry to map
    Entry entry{value, expiry_time, lru_list_.begin()};
    map_[key] = entry;

    // Check if eviction is needed
    evict_if_needed();
}

std::optional<std::string> Cache::get(const std::string& key){
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = map_.find(key);
    if(it == map_.end()){
        misses_++;
        return std::nullopt; // key not found
    }

    auto now = clock::now();
    if(it->second.expiry != clock::time_point::max() && it->second.expiry < now){
        // Key is expired
        lru_list_.erase(it->second.lru_it); // Remove from LRU list
        map_.erase(it); // Remove from map
        misses_++;
        return std::nullopt;  // Return empty optional
    }

    touch_to_front(it); // Move to front of LRU List
    hits_++; 
    return it->second.value; // Return the value
}

bool Cache::erase(const std::string& key){
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto it = map_.find(key);
    if (it == map_.end()) return false;
    lru_list_.erase(it->second.lru_it);
    map_.erase(it);
    return true;
}

size_t Cache::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return map_.size();
}

// PRECONDITION: caller holds mutex_ with a unique_lock
void Cache::touch_to_front(std::unordered_map<std::string, Entry>::iterator it){
    // Move the key to the front of the LRU list
    lru_list_.erase(it->second.lru_it);
    lru_list_.push_front(it->first);
    it->second.lru_it = lru_list_.begin(); 
}

// This method does not check for the TTL, just does raw check if it is present in cache
bool Cache::contains(const std::string& key) const{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return map_.find(key) != map_.end();
}

// This method does not check for the TTL.
std::vector<std::string> Cache::keys() const{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<std::string> result;
    result.reserve(map_.size());
    for(const auto& k : lru_list_){
        result.push_back(k);
    }
    return result;
}

void Cache::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    map_.clear();
    lru_list_.clear();
}

size_t Cache::capacity() const {
    return capacity_;
}

uint64_t Cache::eviction_interval() const {
    return eviction_interval_ms_;
}

size_t Cache::hits() const {
    return hits_.load();
}

size_t Cache::misses() const {
    return misses_.load();
}

// Async eviction
void Cache::eviction_loop(uint64_t interval_ms){
    while(!stop_eviction_.load()){
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));

        auto now = clock::now();
        std::unique_lock<std::shared_mutex> lock(mutex_);

        for(auto it = map_.begin(); it!=map_.end();){
            if (it->second.expiry != clock::time_point::max() && it->second.expiry < now) {
                lru_list_.erase(it->second.lru_it);
                it = map_.erase(it); // erase returns next iterator
            } else {
                ++it;
            }
        }
    }
}