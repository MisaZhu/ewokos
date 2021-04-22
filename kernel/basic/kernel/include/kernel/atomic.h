#ifndef __KERNEL_ATOMIC_H__
#define __KERNEL_ATOMIC_H__

#include <stdint.h>

typedef struct st_atomic {
    volatile uint32_t counter;
} atomic_t;

void atomic_create(atomic_t *atomic);

void atomic_set(atomic_t *atomic, uint32_t val);

uint32_t atomic_get(atomic_t *atomic);

uint32_t atomic_inc(atomic_t *atomic);

uint32_t atomic_dec(atomic_t *atomic);

uint32_t atomic_add(atomic_t *atomic, uint32_t val);

uint32_t atomic_sub(atomic_t *atomic, uint32_t val);

#endif// __KERNEL_ATOMIC_H__
