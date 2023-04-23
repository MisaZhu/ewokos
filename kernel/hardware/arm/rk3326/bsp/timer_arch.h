#ifndef TIMER_ARCH_H
#define TIMER_ARCH_H

#include <stdint.h>

extern uint32_t _timer_tval;

extern void write_cntv_tval(uint32_t tval);

#endif
