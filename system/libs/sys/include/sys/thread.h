#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>

typedef void(*thread_func_t)(void* p);

int thread_create(thread_func_t func, void* p);


#endif
