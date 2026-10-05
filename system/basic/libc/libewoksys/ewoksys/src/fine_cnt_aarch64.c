#include <ewoksys/fine_cnt.h>

/*
 * AArch64 generic-timer virtual count (CNTVCT_EL0), readable from EL0 once the
 * kernel enables it via CNTKCTL_EL1 (arch_enable_user_cnt). This is the counter
 * timer_fine_cnt() samples in the kernel, so the interpolation matches.
 */
inline uint64_t fine_cnt_read(void) {
    uint64_t v;
    __asm__ volatile("mrs %0, CNTVCT_EL0" : "=r"(v) :: "memory");
    return v;
}
