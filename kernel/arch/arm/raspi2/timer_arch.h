#ifndef TIMER_ARCH_H
#define TIMER_ARCH_H

#include <types.h>

extern uint32_t _timer_frq;
void write_cntv_tval(uint32_t val);

#endif
