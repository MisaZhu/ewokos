#ifndef STDLIB_H
#define STDLIB_H

#include <sys/basic_math.h>
#include <stddef.h>

void *malloc(size_t size);
void free(void* ptr);
void* realloc_raw(void* s, uint32_t old_size, uint32_t new_size);
void exit(int status);
int execl(const char* fname, const char* arg, ...);
const char* getenv(const char* name);
int setenv(const char* name, const char* value);

int atoi_base(const char *s, int b);
int atoi(const char *s);
float atof(const char *s);

#endif
