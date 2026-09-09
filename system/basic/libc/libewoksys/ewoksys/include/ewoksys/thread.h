#ifndef THREAD_H
#define THREAD_H

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

#ifdef __cplusplus 
}
#endif

#endif
