#pragma once

#include <stdint.h>

namespace TIMER {

extern uint32_t g_t2irqcount;

void init(void);
uint64_t T2_val(void);
uint64_t T2_ms(void);
} //namespace TIMER