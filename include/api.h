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
    std::shared_ptr<Cache> cache_;
    httplib::Server server_;
};

#endif // API_H