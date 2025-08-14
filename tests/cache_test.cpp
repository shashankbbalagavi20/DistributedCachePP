#include "cache.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

TEST(CacheTest, BasicPutGet) {
    Cache cache(3);
    cache.put("A", "Apple");
    cache.put("B", "Banana");
    cache.put("C", "Cherry");

    EXPECT_EQ(cache.get("A").value(), "Apple");
    EXPECT_EQ(cache.get("B").value(), "Banana");
    EXPECT_EQ(cache.get("C").value(), "Cherry");
}

TEST(CacheTest, LRUEviction) {
    Cache cache(3);
    cache.put("A", "Apple");
    cache.put("B", "Banana");
    cache.put("C", "Cherry");
    cache.get("A"); // Access A so B becomes LRU
    cache.put("D", "Durian"); // Evicts B

    EXPECT_FALSE(cache.get("B").has_value());
    EXPECT_TRUE(cache.get("A").has_value());
    EXPECT_TRUE(cache.get("C").has_value());
    EXPECT_TRUE(cache.get("D").has_value());
}

TEST(CacheTest, TTLExpiry) {
    Cache cache(3);
    cache.put("A", "Apple", 100); // 100 ms
    std::this_thread::sleep_for(150ms);

    EXPECT_FALSE(cache.get("A").has_value()); // Expired
}

TEST(CacheTest, EraseKey) {
    Cache cache(3);
    cache.put("A", "Apple");
    EXPECT_TRUE(cache.erase("A"));
    EXPECT_FALSE(cache.get("A").has_value());
}

TEST(CacheTest, EraseNonExistentKey) {
    Cache cache(3);
    EXPECT_FALSE(cache.erase("NotThere")); // Should not crash
}

TEST(CacheTest, ZeroCapacityCache) {
    Cache cache(0);
    cache.put("A", "Apple");
    EXPECT_FALSE(cache.get("A").has_value()); // Can't store anything
}

TEST(CacheTest, OverwriteKeyWithNewTTL) {
    Cache cache(2);
    cache.put("A", "Apple", 50);
    std::this_thread::sleep_for(30ms);
    cache.put("A", "Apricot", 200); // Overwrite with longer TTL
    std::this_thread::sleep_for(100ms);

    EXPECT_EQ(cache.get("A").value(), "Apricot"); // Should still exist
}

TEST(CacheTest, VeryLargeTTL) {
    Cache cache(2);
    cache.put("A", "Apple", 1000000); // ~16 minutes
    EXPECT_EQ(cache.get("A").value(), "Apple");
}

TEST(CacheTest, GetFromEmptyCache) {
    Cache cache(3);
    EXPECT_FALSE(cache.get("A").has_value());
}

TEST(CacheTest, ConcurrentAccess) {
    Cache cache(5);

    auto writer = [&cache]() {
        for (int i = 0; i < 100; i++) {
            cache.put("Key" + std::to_string(i % 5), "Value" + std::to_string(i));
        }
    };

    auto reader = [&cache]() {
        for (int i = 0; i < 100; i++) {
            auto v = cache.get("Key" + std::to_string(i % 5));
        }
    };

    std::thread t1(writer);
    std::thread t2(reader);
    t1.join();
    t2.join();

    SUCCEED(); // If no crash, we're good
}