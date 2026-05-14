#ifndef EWOKOS_LIBC_MATH_H
#define EWOKOS_LIBC_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#define HUGE_VAL (__builtin_huge_val())
#define INFINITY (__builtin_inff())
#define NAN (__builtin_nanf(""))

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline double fabs(double x) {
	return __builtin_fabs(x);
}

static inline float fabsf(float x) {
	return __builtin_fabsf(x);
}

static inline long double fabsl(long double x) {
	return __builtin_fabsl(x);
}

static inline double sqrt(double x) {
	return __builtin_sqrt(x);
}

static inline float sqrtf(float x) {
	return __builtin_sqrtf(x);
}

double sin(double x);
float sinf(float x);
double cos(double x);
float cosf(float x);
double ceil(double x);

#ifdef __cplusplus
}
#endif

#endif
