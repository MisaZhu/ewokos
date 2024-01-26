#ifndef STDLIB_H
#define STDLIB_H

#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

void __malloc_init(void);
void __malloc_close(void);
void __free__(void* ptr);
void* __malloc__(size_t size);
size_t __size__(void* ptr);
void __mset_thread_lock__(void);
void proc_global_lock(void);
void proc_global_unlock(void);

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void* ptr);
void* realloc(void* s, size_t new_size);

void exit(int status);
int execl(const char* fname, const char* arg, ...);
int execve(const char* fname, char* const arg0[], char* const arg1[]);
const char* getenv(const char* name);
int setenv(const char* name, const char* value);

int atoi_base(const char *s, int b);
int atoi(const char *s);
int64_t atoll(const char *s);
float atof(const char *s);
uint32_t random(void);
int rand(void);


unsigned long strtoul(const char *s, char **ptr, int base);

#ifdef __cplusplus
}
#endif

#endif
