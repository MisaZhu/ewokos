#ifndef THREAD_H
#define THREAD_H

#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void* (*thread_func_t)(void* p);

int  thread_create(thread_func_t func, void* p);
int  thread_get_id(void);

#ifdef __cplusplus 
}
#endif

#endif
