#ifndef KERNEL_FINE_CLOCK_H
#define KERNEL_FINE_CLOCK_H
#include <stdint.h>

/* Smallest integral microsecond/counter period.  19.2 MHz needs 5 us/96
 * ticks, whereas integer MHz clocks need 1 us.  Large periods would make
 * the coarse clock unnecessarily stale, so retain sampling for those. */
static inline uint32_t fine_clock_period_usec(uint32_t hz) {
    uint32_t a=hz,b=1000000u;
    if (!hz) return 0;
    while (b) { uint32_t r=a%b; a=b; b=r; }
    uint32_t period=1000000u/a;
    return period<=10u?period:0;
}

/* Only align BSP clocks that agree with the raw counter.  Approximate BSP
 * conversions and clocks with a different epoch retain their sampled base.
 * The caller publishes both returned values under the same seqlock. */
static inline void fine_clock_anchor(uint32_t hz,uint32_t period,
                                     uint64_t *usec,uint64_t *counter) {
    if (!period) return;
    uint64_t rounded=*usec-*usec%period;
    uint64_t aligned=(rounded/1000000u)*hz+
                     (rounded%1000000u)*hz/1000000u;
    /* Allow one integral period plus the back-to-back sampling overhead. */
    uint64_t slack=(uint64_t)hz*period/1000000u+64u;
    if (*counter>=aligned && *counter-aligned<slack) {
        *usec=rounded;
        *counter=aligned;
    }
}
#endif
