#ifndef STDLIB_H
#define STDLIB_H

#include <types.h>

void* malloc(uint32_t size);

void free(void* p);

int32_t atoi(const char *str);

int32_t setenv(const char* name, const char* value);

const char* getenv(const char* name);

#endif
