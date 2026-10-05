#include <ewoksys/fine_cnt.h>

/*
 * RISC-V rdtime CSR (the mtime time counter), readable from U-mode once the kernel
 * sets scounteren.TM (arch_enable_user_cnt). Same counter timer_fine_cnt() samples
 * in the kernel.
 */
inline uint64_t fine_cnt_read(void) {
    uint64_t v;
#if __riscv_xlen == 64
    __asm__ volatile("rdtime %0" : "=r"(v) :: "memory");
#else
    /* rv32 has no single-instruction 64-bit time read; loop until the high word
       is stable across the low read. */
    uint32_t hi, lo, hi2;
    do {
        __asm__ volatile("rdtimeh %0" : "=r"(hi) :: "memory");
        __asm__ volatile("rdtime  %0" : "=r"(lo) :: "memory");
        __asm__ volatile("rdtimeh %0" : "=r"(hi2) :: "memory");
    } while(hi != hi2);
    v = ((uint64_t)hi << 32) | lo;
#endif
    return v;
}
