# DistributedCache++

[![Unit Tests](https://github.com/shashankbbalagavi20/DistributedCachePP/actions/workflows/ci.yml/badge.svg)](https://github.com/shashankbbalagavi20/DistributedCachePP/actions/workflows/ci.yml)
[![API Tests](https://github.com/shashankbbalagavi20/DistributedCachePP/actions/workflows/api-tests.yml/badge.svg)](https://github.com/shashankbbalagavi20/DistributedCachePP/actions/workflows/api-tests.yml)
[![Docker Support](https://github.com/shashankbbalagavi20/DistributedCachePP/actions/workflows/api-test-docker.yml/badge.svg)](https://github.com/shashankbbalagavi20/DistributedCachePP/actions/workflows/api-test-docker.yml)

**A production-grade C++17 distributed in-memory cache with LRU eviction, TTL expiration, leader–follower replication, sharding, REST API, and Prometheus metrics.**

---

## 📌 Overview

DistributedCache++ is a **high-performance, fault-tolerant caching system** built from scratch in modern C++17.  
It is designed to be a lightweight alternative to Redis/Memcached for learning and experimentation, while implementing real-world distributed systems concepts.

The system supports:
- **O(1) get/put operations**
- **Configurable capacity** with **LRU eviction**
- **Per-key TTL expiration**
- **Thread-safe concurrent access**
- **Leader–follower replication** for fault tolerance
- **Consistent hashing** for horizontal scaling
- **REST API** for integration
- **Prometheus metrics** for observability

---
## 🔍 CI Status

- ✅ **Unit tests** run on **Linux, macOS, and Windows**.  
- ✅ **API integration tests** verify REST endpoints using `curl`.
- ✅ **Docker image** is built and tested in CI.
---

## ✨ Features

✅ **Core Engine**  
- O(1) get/put using hash map + doubly linked list  
- Configurable capacity  
- TTL expiration with background cleanup thread  

✅ **Concurrency**  
- Thread-safe operations using `std::shared_mutex`  
- Fine-grained locking to minimize contention  

✅ **Networking**  
- REST API built with cpp-httplib  
- Endpoints:
  - `GET /cache/<key>`
  - `PUT /cache/<key>`
  - `DELETE /cache/<key>`
  - `GET /metrics` (Prometheus format)

✅ **Distributed Features**  
- Leader–follower replication over HTTP  
- Automatic failover to new leader  
- Sharding via consistent hashing

✅ **Observability**  
- Prometheus metrics: hit/miss ratio, request latency, memory usage, active connections  
- Configurable logging levels

---

## 🛠 Tech Stack

- **Language:** C++17
- **Build System:** CMake 3.15+
- **Core Data Structures:** `std::unordered_map`, `std::list`
- **Networking:** [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- **Serialization:** [nlohmann/json](https://github.com/nlohmann/json)
- **Monitoring:** Prometheus C++ client
- **Testing:** GoogleTest (GTest)
- **Containerization:** Docker
- **Version Control:** Git/GitHub

---

## 🚀 Build & Run

### Prerequisites
- C++17 compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- [CMake](https://cmake.org/) 3.15+
- Git

### Steps
```bash
# Clone repository
git clone https://github.com/<your-username>/DistributedCachePP.git
cd DistributedCachePP

# Create build directory
mkdir build && cd build

# Configure & build
cmake ..
cmake --build .

# --- Run on Linux/Mac ---
./DistributedCachePP --role leader --port 5000

# --- Run on Windows ---
.\Debug\DistributedCachePP.exe --role leader --port 5000

# Run follower nodes
./DistributedCachePP --role follower --port 5001 --leader http://localhost:5000
./DistributedCachePP --role follower --port 5002 --leader http://localhost:5000
```
### 🐳 Run with Docker

You can also run the cache server directly in Docker.

```bash
# Build the image:
docker build -t distributed-cache .

# Run the container:
docker run -p 5000:5000 distributed-cache
```
The server will be available at http://127.0.0.1:5000.

## 📂 Project Structure

DistributedCachePP/\
├── src/                  # Source\
│   ├── main.cpp\
│   ├── cache.cpp\
│   ├── replication.cpp\
│   ├── api.cpp\
│   └── metrics.cpp\
├── include/              # Header files\
│   ├── cache.h\
│   ├── replication.h\
│   ├── api.h\
│   └── metrics.h\
├── tests/                # Unit tests\
│   └── cache_tests.cpp\
├── CMakeLists.txt        # Build configuration\
├── Dockerfile            # Docker Build
└── README.md             # Documentation

## 📜 API Reference

### Get a Value
```bash
GET /cache/<key>
Response: { "value": "<value>" }
```
### Put a Value
```bash
PUT /cache/<key>
Body: { "value": "<value>", "ttl": 30 }
```
### Delete a Value
```bash
DELETE /cache/<key>
```
### Metrics
```bash
GET /metrics
```
Returns Prometheus-formatted metrics.

## 🗺 Roadmap (Completed)

- [x] Single-threaded LRU cache

- [x] TTL expiration

- [x] Thread-safe concurrency

- [x] Async eviction

- [x] REST API

- [x] Leader–follower replication

- [x] Sharding with consistent hashing

- [x] Prometheus metrics

## 📜 License
MIT License – free to use, modify, and distribute.

## 🙌 Acknowledgements
Inspired by Redis, Memcached, and etcd — implemented from scratch in modern C++17 to deepen expertise in systems programming, networking, and distributed systems.