#include "timer.hpp"

#include "ps1/registers.h"

namespace TIMER {

static constexpr int TIMER2_FREQ = F_CPU / 8;

uint32_t g_t2irqcount; // increment in your overflow handler

void init(void) {
    g_t2irqcount = 0;
}

uint64_t T2val(void) {
//    atomic_signal_fence(memory_order_acquire);
    return uint64_t(TIMER_VALUE(2) & 0xffff) | (uint64_t(g_t2irqcount) << 16);
}

uint64_t T2_ms(void) {
    constexpr int tmult = 5;
    constexpr int tdiv  = 21168;
    static_assert(((TIMER2_FREQ * tmult) / tdiv) == 1000);

    return (T2val() * uint64_t(tmult)) / uint64_t(tdiv);
}
}