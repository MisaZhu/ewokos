#ifndef EWOKOS_LIBC_STDLIB_H
#define EWOKOS_LIBC_STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int quot;
	int rem;
} div_t;

typedef struct {
	long quot;
	long rem;
} ldiv_t;

typedef struct {
	long long quot;
	long long rem;
} lldiv_t;

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

int atexit(void (*func)(void));
void abort(void);
void exit(int status);
int abs(int j);
long labs(long j);
int atoi(const char *nptr);
long atol(const char *nptr);
div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);
long strtol(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
float atof(const char *nptr);
double strtod(const char *nptr, char **endptr);
unsigned long strtoul(const char *nptr, char **endptr, int base);
#define RAND_MAX 2147483647
int rand(void);
void srand(unsigned int seed);
long random(void);
void srandom(unsigned int seed);

const char *getenv(const char *name);
int setenv(const char *name, const char *value, ...);
int remove(const char *path);
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
		int (*compar)(const void *, const void *));
void qsort(void *base, size_t nmemb, size_t size,
		int (*compar)(const void *, const void *));

#ifdef __cplusplus
}
#endif

#endif
