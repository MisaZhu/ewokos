#include "kernel/atomic.h"

void atomic_create(atomic_t *atomic) { atomic_set(atomic, 0); }

void atomic_set(atomic_t *atomic, uint32_t val) {
    volatile uint32_t tmp;
    __asm__ __volatile__("@ atomic_set\n\t"
                         "1:  ldrex    %0, [%1]\n\t"    // ldrex : arm的
                         "    strex    %0, %2, [%1]\n\t"// strex
                         "    teq      %0, #0\n\t"
                         "    bne      1b"
                         : "=&r"(tmp)
                         : "r"(&atomic->counter), "r"(val)
                         : "cc");
}

uint32_t atomic_get(atomic_t *atomic) {
    volatile uint32_t result;
    __asm__ __volatile__("@ atomic_get\n\t"
                         "	ldrex	%0, [%1]"
                         : "=&r"(result)
                         : "r"(&atomic->counter));
    return result;
}

uint32_t atomic_inc(atomic_t *atomic) { return atomic_add(atomic, 1); }

uint32_t atomic_dec(atomic_t *atomic) { return atomic_sub(atomic, 1); }

uint32_t atomic_add(atomic_t *atomic, uint32_t val) {
    volatile uint32_t tmp;
    uint32_t result;
    __asm__ __volatile__("@ atomic_add\n\t"
                         "1:  ldrex    %0, [%2]\n\t"
                         "    add      %1, %0, %3\n\t"
                         "    strex    %0, %1, [%2]\n\t"
                         "    teq      %0, #0\n\t"
                         "    bne      1b"
                         : "=&r"(tmp), "=&r"(result)
                         : "r"(&atomic->counter), "r"(val)
                         : "cc");
    return result;
}

uint32_t atomic_sub(atomic_t *atomic, uint32_t val) {
    volatile uint32_t tmp;
    uint32_t result;
    __asm__ __volatile__("@ atomic_sub\n\t"
                         "1:  ldrex    %0, [%2]\n\t"
                         "    sub      %1, %0, %3\n\t"
                         "    strex    %0, %1, [%2]\n\t"
                         "    teq      %0, #0\n\t"
                         "    bne      1b"
                         : "=&r"(tmp), "=&r"(result)
                         : "r"(&atomic->counter), "r"(val)
                         : "cc");
    return result;
}
