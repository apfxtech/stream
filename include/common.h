#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <algorithm>
#include <memory>

#ifndef ARDUINO
#include <unistd.h>
#endif

// в случае отсутствия debug.h
// определяем пустые макросы логирования
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#define LOG_ERROR(msg) ((void)0)
#define LOG_ERROR_F(fmt, ...) ((void)0)
#define LOG_WARN(msg) ((void)0)
#define LOG_WARN_F(fmt, ...) ((void)0)
#define LOG_INFO(msg) ((void)0)
#define LOG_INFO_F(fmt, ...) ((void)0)
#define LOG_DEBUG(msg) ((void)0)
#define LOG_DEBUG_F(fmt, ...) ((void)0)
#define LOG_HEX(prefix, data, len) ((void)0)
#define DEBUG_ONLY(code) ((void)0)
#define LOG_TRACE(msg) ((void)0)
#define LOG_TRACE_F(fmt, ...) ((void)0)
#define TRACE_ONLY(code) ((void)0)
#endif

#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR
#endif

#ifndef ARDUINO
#include <chrono>

static inline uint32_t millis()
{
    using namespace std::chrono;
    static auto start = steady_clock::now();
    return duration_cast<milliseconds>(steady_clock::now() - start).count();
}
#endif