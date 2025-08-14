#pragma once
#ifndef CACHE_H
#define CACHE_H

#include <string>
#include <unordered_map>
#include <list>
#include <chrono>
#include <optional>
#include <shared_mutex>

/**
 * Cache class with:
 * - LRU eviction (Least Recently Used)
 * - TTL expiration (per key)
 * - O(1) average complexity for get/put
 * 
 * Note: Single-threaded version(no mutexes yet)
 */
class Cache {
public:
    /**
     * Constructor
     * @param capacity Maximum number of items in the cache
     */
    explicit Cache(size_t capacity);

    /**
     * Put key-value pair into the cache with optional TTL (in milliseconds)
     * ttl_ms = 0 means no expiry
     */
    void put(const std::string& key, const std::string& value, uint64_t ttl_ms = 0);

    /**
     * Get value if present and not expired
     * @return std::optional<std::string> - contains value if hit, empty if miss
     */
    std::optional<std::string> get(const std::string& key);

    /**
     * Remove a key from cache
     * @return true if removed, false if not found
     */
    bool erase(const std::string& key);

    /**
     * Current number of entries
     */
    size_t size() const;

    /**
     * Debug helper: prints current cache order (MRU -> LRU)
     */
    void print_state() const;

private:
    using clock = std::chrono::steady_clock;

    // Internal entry struct
    struct Entry
    {
        std::string value;                        // Stored value
        clock::time_point expiry;                // Expiration time
        std::list<std::string>::iterator lru_it; // Iterator pointing to position in LRU list
    };
    
    /**
     * Moves accessed/updated key to the front of LRU list
     */
    void touch_to_front(typename std::unordered_map<std::string, Entry>::iterator it);

    /**
     * Removes the least recently used key if capacity exceeded
     */
    void evict_if_needed();

    mutable std::shared_mutex mutex_; // Protects map_, lru_list_, capacity_
    size_t capacity_; // Max allowed entries
    std::unordered_map<std::string, Entry> map_; // key -> Entry
    std::list<std::string> lru_list_; // keys in MRU -> LRU order
};

#endif // CACHE_H