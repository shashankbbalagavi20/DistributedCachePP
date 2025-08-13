#include "cache.h"
#include <iostream>

int main() {
    Cache cache(3); // capacity = 3

    cache.put("A", "Apple");
    cache.put("B", "Banana");

    auto val = cache.get("A");
    if (val) {
        std::cout << "A: " << *val << "\n";
    } else {
        std::cout << "A: MISS\n";
    }

    return 0;
}