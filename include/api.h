#pragma once
#ifndef API_H
#define API_H

#include "cache.h"
#include "httplib.h"
#include <memory>
#include <string>

/**
 * REST API wrapper around Cache
 */
class CacheAPI {
public:
    /**
     * Constructor
     * @param cache Shared pointer to Cache instance
     */
    explicit CacheAPI(std::shared_ptr<Cache> cache);

    /**
     * Start the HTTP server
     * @param host Host to bind (default: "0.0.0.0")
     * @param port Port to bind
     */
    void start(const std::string& host, int port);

private:
    /**
     * Log an incoming request with method, path, and status code
     */
    void logRequest(const std::string& method, const std::string& path, int status);
    std::shared_ptr<Cache> cache_;
    httplib::Server server_;
};

#endif // API_H