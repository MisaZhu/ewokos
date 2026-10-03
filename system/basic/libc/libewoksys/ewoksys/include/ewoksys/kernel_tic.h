#ifndef KERNEL_TIC_H 
#define KERNEL_TIC_H 

#include <stdint.h>

#ifdef __cplusplus 
extern "C" {
#endif

int32_t kernel_tic(uint32_t* sec, uint64_t* usec);
int32_t kernel_tic32(uint32_t* sec, uint32_t* usec_hi, uint32_t* usec_low);
uint64_t kernel_tic_ms(int zone);

/*
 * Nanosecond clock, interpolated from the kernel-published counter base (see
 * vsyscall_fine_clock_t in sysinfo.h). No syscall: it reads the free-running
 * hardware counter directly and extends the tick-quantized kernel_usec.
 *
 * kernel_tic_nsec() returns -1 when _vsyscall_info is not mapped yet, the same
 * as kernel_tic(). It never fails on platforms without a user-readable counter
 * - it simply reports the tick-quantized value scaled up, so callers do not
 * need two code paths. Use kernel_tic_fine_available() only to decide whether
 * spinning for sub-tick precision is worthwhile.
 */
int32_t kernel_tic_nsec(uint64_t* nsec);

/* 1 when the fine clock is really sub-tick accurate on this platform, 0 when
   it just mirrors kernel_usec. */
int32_t kernel_tic_fine_available(void);

/* frequency in Hz of the counter kernel_tic_nsec() interpolates from, or 0 when
   there is none. Only useful for reporting a resolution (see clock_getres). */
uint32_t kernel_tic_fine_hz(void);

/* measured scheduler tick period in usec (0 if not yet known) */
uint32_t kernel_tic_tick_usec(void);

/*
 * Raw-counter timing, for intervals that must not be shortened by a clock
 * republication.
 *
 * kernel_tic_nsec() interpolates from the (kernel_usec, base_cnt) pair the
 * kernel republishes every tick, so its offset against real time is re-derived
 * 1000 times a second and can shift by a microsecond or so when it is. That is
 * invisible to a timestamp but fatal to a deadline: a sleeper spinning on it can
 * be pushed past its target and wake EARLY. These two functions measure against
 * the free-running counter itself instead, which nothing republishes.
 *
 * kernel_tic_fine_cnt() snapshots the counter, returning -1 when this platform
 * has no user-readable one (x86, pre-ARMv7). kernel_tic_spin_until() then
 * busy-waits until nsec nanoseconds have elapsed since that snapshot, bounded by
 * max_iters so a stuck counter cannot hang the caller; it returns 0 on success
 * and -1 if the counter is unusable or the cap tripped, in which case the caller
 * should sleep coarsely rather than return short.
 */
int32_t kernel_tic_fine_cnt(uint64_t* cnt);
int32_t kernel_tic_spin_until(uint64_t from_cnt, uint64_t nsec, uint32_t max_iters);

#ifdef __cplusplus
} 
#endif

#endif
