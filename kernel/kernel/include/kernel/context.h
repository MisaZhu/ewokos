#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <arch_context.h>

typedef struct {
    uint32_t  state;
    uint32_t  sleep_counter;
    context_t ctx;
} saved_state_t;

#endif
