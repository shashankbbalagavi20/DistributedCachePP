#include "api.h"
#include <iostream>
#include <memory>

int main() {
    auto cache = std::make_shared<Cache>(100); // capacity 100
    CacheAPI api(cache);

    std::cout << "🚀 Starting server on http://localhost:5000" << std::endl;
    api.start("0.0.0.0", 5000);
    return 0;
}