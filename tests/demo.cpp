#include "cache.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    Cache cache(3); // capacity = 3

    std::cout << "=== Basic put/get ===\n";
    cache.put("A", "Apple");
    cache.put("B", "Banana");
    cache.put("C", "Cherry");
    cache.print_state();

    std::cout << "Get A: " << *cache.get("A") << "\n";
    cache.print_state(); // A should now be MRU

    std::cout << "\n=== LRU eviction ===\n";
    cache.put("D", "Dates"); // Evicts B (LRU)
    cache.print_state();
    auto resB = cache.get("B");
    std::cout << "Get B: " << (resB ? *resB : "MISS") << "\n";

    std::cout << "\n=== TTL expiry ===\n";
    cache.put("E", "Elderberry", 1000); // 1 sec TTL
    cache.print_state();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    auto resE = cache.get("E");
    std::cout << "Get E after TTL: " << (resE ? *resE : "MISS") << "\n";
    cache.print_state();

    std::cout << "\n=== Erase ===\n";
    cache.put("F", "Fig");
    cache.print_state();
    cache.erase("A");
    cache.print_state();

    return 0;
}
