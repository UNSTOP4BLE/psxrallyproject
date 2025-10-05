#include "../timer.hpp"

#include "ps1/registers.h"

namespace ENGINE::PSX {

constexpr int TIMER2_FREQ = F_CPU / 8;

PSXTimer::PSXTimer(void) {
    t2irqcount = 0;
    TIMER_CTRL(2) = TIMER_CTRL_IRQ_ON_OVERFLOW 
                  | TIMER_CTRL_IRQ_REPEAT 
                  | TIMER_CTRL_PRESCALE;         // clock source = 2 (SysClk/8)
}

uint64_t PSXTimer::getMS(void) {
    constexpr int tmult = 5;
    constexpr int tdiv  = 21168;
    static_assert(((TIMER2_FREQ * tmult) / tdiv) == 1000);

    return (getT2_value() * uint64_t(tmult)) / uint64_t(tdiv);
};

uint64_t PSXTimer::getT2_value(void) {
    __atomic_signal_fence(__ATOMIC_ACQUIRE);
    return uint64_t(TIMER_VALUE(2) & 0xffff) | (uint64_t(t2irqcount) << 16);
};

} 