#ifndef STDLIB_H
#define STDLIB_H

#include <types.h>

void* malloc(uint32_t size);

void free(void* p);

int32_t atoi(const char *str);

#endif
