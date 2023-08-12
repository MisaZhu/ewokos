#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    TIMER_SET = 0,
    TIMER_REMOVE
};

typedef void (*timer_handle_t)(void);
uint32_t timer_set(uint32_t usec, timer_handle_t handle);
void     timer_remove(uint32_t id);

#ifdef __cplusplus
}
#endif
#endif