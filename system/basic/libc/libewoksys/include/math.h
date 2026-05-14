#ifndef EWOKOS_LIBC_MATH_H
#define EWOKOS_LIBC_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#define HUGE_VAL (__builtin_huge_val())
#define INFINITY (__builtin_inff())
#define NAN (__builtin_nanf(""))
#define isnan(x) __builtin_isnan(x)
#define isinf(x) __builtin_isinf_sign(x)
#define isfinite(x) __builtin_isfinite(x)

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
double tan(double x);
float tanf(float x);
double acos(double x);
float acosf(float x);
double asin(double x);
float asinf(float x);
double atan(double x);
float atanf(float x);
double sinh(double x);
float sinhf(float x);
double cosh(double x);
float coshf(float x);
double tanh(double x);
float tanhf(float x);
double asinh(double x);
float asinhf(float x);
double acosh(double x);
float acoshf(float x);
double atanh(double x);
float atanhf(float x);
double atan2(double y, double x);
float atan2f(float y, float x);
double exp(double x);
float expf(float x);
double exp2(double x);
float exp2f(float x);
double pow(double x, double y);
float powf(float x, float y);
double log(double x);
float logf(float x);
double log2(double x);
float log2f(float x);
double log10(double x);
float log10f(float x);
double fmod(double x, double y);
float fmodf(float x, float y);
double modf(double x, double *iptr);
float modff(float x, float *iptr);
double fmin(double x, double y);
float fminf(float x, float y);
double fmax(double x, double y);
float fmaxf(float x, float y);
double ldexp(double x, int exp);
float ldexpf(float x, int exp);
double frexp(double x, int *exp);
float frexpf(float x, int *exp);
double hypot(double x, double y);
float hypotf(float x, float y);
double ceil(double x);
float ceilf(float x);
double floor(double x);
float floorf(float x);
double rint(double x);
float rintf(float x);
double trunc(double x);
float truncf(float x);
double round(double x);
float roundf(float x);
double nearbyint(double x);
float nearbyintf(float x);

#ifdef __cplusplus
}
#endif

#endif
