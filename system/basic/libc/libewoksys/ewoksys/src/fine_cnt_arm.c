#include <ewoksys/fine_cnt.h>

/*
 * ARMv7-A generic-timer virtual count via the CP14 mrrc (CNTVCT), readable from PL1
 * once the kernel enables it through CNTKCTL (arch_enable_user_cnt). Same counter
 * timer_fine_cnt() samples in the kernel.
 *
 * This single file is compiled for every ARCH=arm target, which spans ARMv7 (raspi
 * and friends, built -march=armv7-a) down to ARMv5 (lego.ev3, ARM926EJ-S, built
 * -march=armv5te). Pre-v7 cores have no generic timer, and the CP14 mrrc is not a
 * legal v5 instruction - emitting it would be an assembly error before ever reaching
 * the missing hardware. So it is guarded on the compiler-predefined arch macros and
 * pre-v7 returns 0, which kernel_tic.c discards anyway because FINE_CNT_READABLE is
 * 0 there (the header's gate makes the same v7 test). ARCH_VER=v7 is what adds
 * -march=armv7-a to the userspace build, hence testing the macros rather than a
 * build variable.
 */
inline uint64_t fine_cnt_read(void) {
#if defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7_) || \
    (defined(__ARM_ARCH) && (__ARM_ARCH >= 7))
    uint64_t v;
    __asm__ volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r"(v) :: "memory"); /* CNTVCT */
    return v;
#else
    return 0;
#endif
}
