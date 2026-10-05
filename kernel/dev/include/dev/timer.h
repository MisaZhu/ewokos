#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern void timer_set_interval(uint32_t id, uint32_t times_per_sec);

extern void timer_clear_interrupt(uint32_t id);

extern void timer_init(void);

extern uint64_t timer_read_sys_usec(void); //read usec

/*
 * Raw free-running counter backing timer_read_sys_usec(), for publication to
 * userspace so libc can interpolate the clock between scheduler ticks (the
 * tick is ~976us at the default timer_freq=1024, far too coarse for
 * nanosleep/clock_gettime).
 *
 * Stores the current counter value in *cnt and returns its frequency in Hz,
 * or returns 0 when this platform has no counter userspace may read directly
 * (x86's PIT, ev3's Davinci TIM). In that case the kernel publishes
 * fine_cnt_hz = 0 and libc falls back to the tick-quantized clock, so the
 * counter returned here MUST be the same one userspace reads - a mismatch
 * silently corrupts every interpolated timestamp.
 *
 * Implemented per-architecture in kernel/platform/<arch>/arch/common/src/irq.c
 * (weak, so a BSP may override a board with an unusual counter frequency).
 */
extern uint32_t timer_fine_cnt(uint64_t* cnt);

#endif
