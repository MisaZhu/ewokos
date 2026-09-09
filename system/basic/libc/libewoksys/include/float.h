#ifndef EWOKOS_LIBC_FLOAT_H
#define EWOKOS_LIBC_FLOAT_H

#define FLT_RADIX 2

#define FLT_MANT_DIG 24
#define DBL_MANT_DIG 53
#define LDBL_MANT_DIG 64

#define FLT_DIG 6
#define DBL_DIG 15
#define LDBL_DIG 18

#define FLT_EPSILON 1.19209290e-07F
#define DBL_EPSILON 2.2204460492503131e-16
#define LDBL_EPSILON 1.0842021724855044340e-19L

#define FLT_MIN_10_EXP (-37)
#define DBL_MIN_10_EXP (-307)
#define LDBL_MIN_10_EXP (-4931)

#define FLT_MIN_EXP (-125)
#define DBL_MIN_EXP (-1021)
#define LDBL_MIN_EXP (-16381)

#define FLT_MAX_EXP 128
#define DBL_MAX_EXP 1024
#define LDBL_MAX_EXP 16384

/*
 * The maximum base-10 exponent: the largest n such that 10^n is representable.
 * These three were simply absent from this header, which is invisible until
 * something asks - qspinbox.cpp:992 does, and failed with
 * "'DBL_MAX_10_EXP' was not declared in this scope".
 *
 * Taken from the compiler's own builtins rather than written as literals, and
 * that choice is deliberate: this header sits in $EWOK_SDK/include, which the Qt
 * mkspec puts in CFLAGS ahead of every INCLUDEPATH entry, so it shadows GCC's
 * float.h.  A shadowing header that hardcodes values is a header that is wrong on
 * every architecture but the one it was written on.  The builtins are always
 * right for the target actually being compiled for - on aarch64 they are 38, 308
 * and 4932.
 *
 * Related, and left alone on purpose: LDBL_MANT_DIG above says 64, which is x87
 * 80-bit extended precision, but aarch64 GCC reports __LDBL_MANT_DIG__ as 113
 * (IEEE binary128), with LDBL_MAX_EXP 16384 and LDBL_MIN_EXP -16381 also being
 * the x87 numbers.  <limits> is unaffected because numeric_limits<long double>
 * is built from the __LDBL_*__ builtins directly.  Fixing the long double block
 * means either rewriting all of it from builtins or making it architecture
 * conditional, which is a larger change than adding three missing macros and
 * should not ride along with them.
 */
#ifdef __FLT_MAX_10_EXP__
#define FLT_MAX_10_EXP __FLT_MAX_10_EXP__
#else
#define FLT_MAX_10_EXP 38
#endif

#ifdef __DBL_MAX_10_EXP__
#define DBL_MAX_10_EXP __DBL_MAX_10_EXP__
#else
#define DBL_MAX_10_EXP 308
#endif

#ifdef __LDBL_MAX_10_EXP__
#define LDBL_MAX_10_EXP __LDBL_MAX_10_EXP__
#else
#define LDBL_MAX_10_EXP 4932
#endif

#define FLT_MIN 1.17549435e-38F
#define DBL_MIN 2.2250738585072014e-308
#define LDBL_MIN 3.3621031431120935063e-4932L

#define FLT_MAX 3.40282347e+38F
#define DBL_MAX 1.7976931348623157e+308
#define LDBL_MAX 1.1897314953572317650e+4932L

#endif
