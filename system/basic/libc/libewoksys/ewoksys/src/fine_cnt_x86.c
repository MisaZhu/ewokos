#include <ewoksys/fine_cnt.h>

/*
 * The calibrated TSC, matching the counter timer_fine_cnt() samples in the kernel
 * (machines/x86/kernel/bsp/timer.c). Userland rdtsc is permitted because the kernel
 * clears CR4.TSD (arch_enable_user_cnt).
 *
 * rdtsc is not serializing, but the "memory" clobber keeps the compiler from
 * reordering it across the seqlock fences in the caller; a slightly out-of-order
 * read only ever overshoots a spin deadline, which kernel_tic_spin_until treats as
 * acceptable (late is recoverable, early is not). Gated at runtime by fine_cnt_hz:
 * a BSP that published no calibrated frequency never reaches this path.
 */
inline uint64_t fine_cnt_read(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}
