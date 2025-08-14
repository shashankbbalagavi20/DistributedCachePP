#include "cache.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

// Basic put/get
TEST(CacheTest, BasicPutGet) {
    Cache cache(3);
    cache.put("A", "Apple");
    cache.put("B", "Banana");
    cache.put("C", "Cherry");

    auto val = cache.get("A");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "Apple");
}

// LRU eviction test
TEST(CacheTest, LRUEviction) {
    Cache cache(2);
    cache.put("A", "Apple");
    cache.put("B", "Banana");
    cache.get("A"); // make A MRU
    cache.put("C", "Cherry"); // evict B

    EXPECT_FALSE(cache.get("B").has_value());
    EXPECT_TRUE(cache.get("A").has_value());
    EXPECT_TRUE(cache.get("C").has_value());
}

// TTL expiry test
TEST(CacheTest, TTLExpiry) {
    Cache cache(2);
    cache.put("A", "Apple", 500); // 0.5s TTL
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    EXPECT_FALSE(cache.get("A").has_value());
}

// Erase test
TEST(CacheTest, EraseWorks) {
    Cache cache(2);
    cache.put("A", "Apple");
    EXPECT_TRUE(cache.erase("A"));
    EXPECT_FALSE(cache.get("A").has_value());
}

// Thread-safety smoke test
TEST(CacheTest, ThreadSafetySmoke) {
    Cache cache(5);

    auto writer = [&]() {
        for (int i = 0; i < 1000; ++i)
            cache.put("key" + std::to_string(i % 5), "val");
    };
    auto reader = [&]() {
        for (int i = 0; i < 1000; ++i)
            cache.get("key" + std::to_string(i % 5));
    };

    std::thread t1(writer);
    std::thread t2(reader);
    std::thread t3(writer);
    std::thread t4(reader);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    SUCCEED(); // No crash means thread safety holds
}
