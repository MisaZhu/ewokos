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
long long llabs(long long j);
int atoi(const char *nptr);
long atol(const char *nptr);
div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);
long strtol(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
double atof(const char *nptr);
double strtod(const char *nptr, char **endptr);
unsigned long strtoul(const char *nptr, char **endptr, int base);
#define RAND_MAX 2147483647
int rand(void);
void srand(unsigned int seed);
long random(void);
void srandom(unsigned int seed);

char *getenv(const char *name);
int __ewok_setenv_impl(const char *name, const char *value, int overwrite);
int remove(const char *path);
int mkstemp(char *template);
char *tempnam(const char *dir, const char *pfx);
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
		int (*compar)(const void *, const void *));
void qsort(void *base, size_t nmemb, size_t size,
		int (*compar)(const void *, const void *));

static inline int __ewok_setenv_default(const char *name, const char *value) {
	return __ewok_setenv_impl(name, value, 1);
}

#define __EWOK_SETENV_SELECT(_1, _2, _3, NAME, ...) NAME
#define setenv(...) __EWOK_SETENV_SELECT(__VA_ARGS__, __ewok_setenv_impl, __ewok_setenv_default)(__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
