#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

extern void __irq_enable(void);
extern void __irq_disable(void);

extern uint32_t __int_off(void);
extern void __int_on(uint32_t);
extern void __enable_scu(void);

extern void _delay(uint32_t count);
extern void _delay_usec(uint64_t count);
extern void _delay_msec(uint32_t count);

extern void set_translation_table_base(uint32_t);
extern void flush_tlb(void);

#ifdef KERNEL_SMP
#include <kernel/smp/ipi.h>

extern void kernel_lock_init(void);
extern int32_t kernel_lock_check(void);
extern void kernel_lock(void);
extern void kernel_unlock(void);
#else
#define kernel_lock_init()
#define kernel_lock_check() 0
#define kernel_lock()
#define kernel_unlock()
#endif

extern void halt(void);
#endif
