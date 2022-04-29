#ifndef THREAD_H
#define THREAD_H

#include <sys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void* (*thread_func_t)(void* p);

int  thread_create(thread_func_t func, void* p);
int  thread_get_id(void);

void thread_safe_set(int32_t* to, int32_t v);
void thread_lock(void);
void thread_unlock(void);

#ifdef __cplusplus 
}
#endif

#endif
