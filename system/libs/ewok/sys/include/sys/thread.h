#ifndef THREAD_H
#define THREAD_H

#include <sys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void* (*thread_func_t)(void* p);

int thread_create(thread_func_t func, void* p);

#ifdef __cplusplus 
}
#endif

#endif
