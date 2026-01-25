#include "../timer.hpp"
#include <chrono>

namespace ENGINE::GENERIC {

    static std::chrono::steady_clock::time_point start;
    
    ChronoTimer::ChronoTimer(void) {
        start = std::chrono::steady_clock::now();
    }

    uint64_t ChronoTimer::getMS(void) {
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        return elapsed.count();
    };
} 