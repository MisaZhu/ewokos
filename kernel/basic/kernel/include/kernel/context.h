#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} context_t;

typedef struct {
    uint32_t  state;
    int32_t   block_by;
    uint32_t  block_event;
    context_t ctx;
} saved_state_t;

#endif
