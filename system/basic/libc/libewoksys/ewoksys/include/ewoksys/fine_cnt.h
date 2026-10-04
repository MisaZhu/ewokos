#ifndef __EWOKSYS_FINE_CNT_H__
#define __EWOKSYS_FINE_CNT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Userspace reader for the free-running hardware counter the kernel interpolates
 * its clock from (the BSP's timer_fine_cnt()). It MUST be the very same counter,
 * otherwise every timestamp derived in kernel_tic.c is silently wrong.
 *
 * The implementation is arch-specific and lives in fine_cnt_<arch>.c, pulled into
 * the build exactly like syscall_<arch>.S / setjmp_<arch>.S: one fine_cnt_$(ARCH).o
 * entry in LIB_EWOKSYS_OBJS selects the right translation unit per target. This is
 * the single header for the whole feature - it only declares the entry point and
 * resolves the FINE_CNT_READABLE compile-time gate.
 *
 * FINE_CNT_READABLE has to be a macro resolved here (not something the .c can
 * provide) because kernel_tic.c uses it in #if to compile the fine-clock paths out
 * entirely on targets with no readable counter. It is the compile-time half of the
 * availability decision, checked next to the kernel's runtime fine_cnt_hz before
 * anything spins on this clock, so a platform whose kernel has a counter but whose
 * libc was built without one degrades to the tick clock instead of spinning on a
 * constant that never advances.
 *
 * The gate keys off compiler-predefined arch macros, which agree with the $(ARCH)
 * that picks fine_cnt_$(ARCH).c: aarch64/riscv/x86 are always readable; "arm" spans
 * ARMv7 (CNTVCT via CP14 mrrc) down to pre-v7 lego.ev3 (no counter), so the v7 test
 * below mirrors the version split that fine_cnt_arm.c makes internally. aarch64 is
 * listed first only for readability - the macros are mutually exclusive.
 */
#if defined(__aarch64__) || defined(__riscv) || defined(__x86_64__) || \
    defined(__i386__) || defined(__ARM_ARCH_7A__) || \
    defined(__ARM_ARCH_7_) || (defined(__ARM_ARCH) && (__ARM_ARCH >= 7))
#define FINE_CNT_READABLE 1
#else
#define FINE_CNT_READABLE 0
#endif

uint64_t fine_cnt_read(void);

#ifdef __cplusplus
}
#endif

#endif
