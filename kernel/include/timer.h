#ifndef TIMER_H
#define TIMER_H

#include <types.h>

void timer_init();
void timer_start();

void timer_set_interval(uint32_t intervalMicrosecond);
void timer_clear_interrupt(void);

void cpu_tick(uint32_t* sec, uint32_t* msec);

#endif
