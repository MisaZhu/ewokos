#ifndef THREAD_H
#define THREAD_H

#include <types.h>

typedef void (*thread_func_t)(void*p); 

int32_t thread_raw(thread_func_t func, void* p);

#endif
