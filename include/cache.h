#pragma once
#ifndef CACHE_H
#define CACHE_H

#include <string>
#include <unordered_map>
#include <list>
#include <chrono>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <vector>

/**
 * Thread-safe Cache with:
 * - LRU eviction (Least Recently Used)
 * - TTL expiration (per key, in ms)
 * - Async background eviction (thread removes expired keys periodically)
 * - O(1) average complexity for get/put
 * - Basic metrics: cache hits & misses
 */
class Cache {
public:
    using clock = std::chrono::steady_clock;

    /**
     * Constructor
     * @param capacity             Maximum number of items in the cache
     * @param eviction_interval_ms Interval (ms) for background async eviction
     */
    explicit Cache(size_t capacity, uint64_t eviction_interval_ms = 100);

    /**
     * Destructor - stops background eviction thread.
     */
    virtual ~Cache();

    // ---------------- Public API ----------------

    /**
     * Insert or update a key-value pair with optional TTL.
     * @param key       Key string
     * @param value     Value string
     * @param ttl_ms    Time-to-live in ms (0 = no expiry)
     */
    void put(const std::string& key, const std::string& value, uint64_t ttl_ms = 0);

    /**
     * Get value if present and not expired.
     * Increments hit/miss counters for metrics.
     * @param key Key to fetch
     * @return std::optional containing value if hit, empty if miss
     */
    std::optional<std::string> get(const std::string& key);

    /**
     * Remove a key from cache.
     * @param key Key to erase
     * @return true if key was removed, false if not found
     */
    bool erase(const std::string& key);

    /**
     * @return Current number of entries in the cache
     */
    size_t size() const;

    /**
     * Raw check: returns true if key exists in map (ignores TTL).
     * Mainly useful for testing async eviction.
     */
    bool contains(const std::string& key) const;

    /**
     * Snapshot of all keys currently in cache (ignores TTL).
     */
    std::vector<std::string> keys() const;

    /** 
    * Clear all the contents in map_ and lru_list_
    */
    void clear();

    /**
    * @return capacity of the cache
    */ 
    size_t capacity() const;

    /**
    * @return the time in which async eviction thread is running
    */ 
    uint64_t eviction_interval() const;

    /**
     * @return Number of successful cache hits
     */
    size_t hits() const;

    /**
     * @return Number of cache misses
     */
    size_t misses() const;

private:
    // ---------------- Internal types ----------------

    struct Entry {
        std::string value;                        ///< Stored value
        clock::time_point expiry;                 ///< Expiration time
        std::list<std::string>::iterator lru_it;  ///< Iterator pointing into LRU list
    };

    // ---------------- Internal helpers ----------------

    /// Move accessed/updated key to front of LRU list.
    void touch_to_front(typename std::unordered_map<std::string, Entry>::iterator it);

    /// Remove least recently used key if capacity exceeded.
    void evict_if_needed();

    /// Background eviction loop: periodically removes expired keys.
    void eviction_loop(uint64_t interval_ms);

    // ---------------- Data members ----------------
    mutable std::shared_mutex mutex_;               ///< Protects map_, lru_list_, capacity_
    size_t capacity_;                               ///< Max allowed entries
    std::unordered_map<std::string, Entry> map_;    ///< key -> Entry
    std::list<std::string> lru_list_;               ///< Keys in MRU → LRU order
    
    // Async eviction members
    std::thread eviction_thread_;
    std::atomic<bool> stop_eviction_{false};
    uint64_t eviction_interval_ms_;

    // Metrics
    std::atomic<size_t> hits_{0};               ///< Count of cache hits
    std::atomic<size_t> misses_{0};            ///< Count of cache misses
};

#endif // CACHE_H