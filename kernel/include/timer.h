#ifndef TIMER_H
#define TIMER_H

#include <types.h>

void timer_set_interval(uint32_t intervalMicrosecond);
void timer_clear_interrupt(void);
void timer_init();
uint32_t timer_read();
void timer_wait(uint32_t delay);

#endif
