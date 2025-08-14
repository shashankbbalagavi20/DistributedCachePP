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

TEST(CacheTest, OverwriteKeyWithNewTTL)
{
    using namespace std::chrono;
    Cache cache(2);

    // Initial TTL short
    cache.put("A", "Apple", 50);
    std::this_thread::sleep_for(20ms);

    // Overwrite BEFORE first TTL expires, with a MUCH longer TTL
    cache.put("A", "Apricot", 1000);  // 1s

    // Immediately confirm overwrite took effect
    auto v1 = cache.get("A");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, "Apricot");

    // Wait well under the new TTL to avoid flakiness
    std::this_thread::sleep_for(200ms);
    auto v2 = cache.get("A");
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(*v2, "Apricot");

    // Prove it does expire after new TTL:
    // Sleep until safely past the 1s TTL from the second put.
    std::this_thread::sleep_for(900ms); // total ~1.1s after second put
    auto v3 = cache.get("A");
    EXPECT_FALSE(v3.has_value());
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