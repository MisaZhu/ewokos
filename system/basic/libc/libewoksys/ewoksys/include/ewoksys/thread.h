#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>
#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void* (*thread_func_t)(void* p);

int  thread_create(thread_func_t func, void* p);
int  thread_get_id(void);

/*
 * Releases the calling task's thread_local storage. thread_create's teardown
 * calls it once the thread function has returned and the pthread_key
 * destructors have run - those destructors may themselves read thread_locals,
 * so this has to come last. Defined for every architecture, a no-op where the
 * kernel carries no TLS base.
 */
void __ewok_emutls_thread_exit(void);

/*
 * Per-task cache of the kernel thread id, held in the TLS block that hangs off
 * TPIDR_EL0 (see src/tls/emutls.c). thread_get_id()/pthread_self() read it so
 * they answer with a register read instead of a syscall - they are called from
 * the heap lock on every malloc/free. Defined for every architecture; -1/no-op
 * where the kernel carries no TLS base, so thread_get_id() keeps trapping there.
 */
int32_t __ewok_tls_get_tid(void);
void    __ewok_tls_cache_tid(int32_t tid);
void    __ewok_tls_init_tid(int32_t tid);
/* Clears TPIDR_EL0 so a new image starts with fresh thread_locals; _start only. */
void    __ewok_tls_reset(void);

#ifdef __cplusplus 
}
#endif

#endif
