#pragma once
#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <ctime>

/**
 * Cross platform safe localtime wrapper.
 * windows -> localtime_s
 * Linux/Unix -> localtime_r
 */
inline std::tm safe_localtime(std::time_t time){
    std::tm tm_buf{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    return tm_buf;
}

#endif // TIME_UTILS_H