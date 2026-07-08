#pragma once
#include <chrono>

namespace Time {
    inline auto start_time = std::chrono::steady_clock::now();
    inline long long allocated_ms = 0;
    inline bool time_up = false;

    inline void init(long long ms) {
        start_time = std::chrono::steady_clock::now();
        allocated_ms = ms;
        time_up = false;
    }

    // Check the clock, but only once every 2048 nodes to protect NPS
    inline void check(uint64_t nodes) {
        if ((nodes & 2047) == 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
            if (elapsed >= allocated_ms) {
                time_up = true;
            }
        }
    }
}