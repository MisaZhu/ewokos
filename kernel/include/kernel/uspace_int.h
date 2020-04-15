#ifndef USPACE_INT_H
#define USPACE_INT_H

#include "usinterrupt.h"
#include <_types.h>

extern void uspace_interrupt_init(void);

extern int32_t uspace_interrupt_register(int32_t int_id);
extern int32_t uspace_interrupt_get_pid(int32_t int_id);
extern void uspace_interrupt_unregister(int32_t int_id);

#endif
