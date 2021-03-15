#pragma once
#include <iostream>
#include <sstream>

// #define DEBUG_MODE

#ifdef DEBUG_MODE
#define debug(s) std::cerr << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << "(...) [DEBUG]:" << s << "\n";
#else
#define debug(stream) \
    do {              \
    } while (false)
#endif

#define throttled(rate, s)                                                                                         \
    do {                                                                                                           \
        static auto start = std::chrono::steady_clock::now();                                                      \
        auto now = std::chrono::steady_clock::now();                                                               \
        if (now - start > std::chrono::duration<float>(rate)) {                                                    \
            std::cerr << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << "(...) [THROTTLED]: " << s << "\n"; \
            start = now;                                                                                           \
        }                                                                                                          \
    } while (false)
