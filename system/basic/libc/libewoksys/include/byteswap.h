#ifndef EWOKOS_LIBC_BYTESWAP_H
#define EWOKOS_LIBC_BYTESWAP_H

/*
 * <byteswap.h> - bswap_16 / bswap_32 / bswap_64.
 *
 * The glibc name for byte reversal.  newlib has no byteswap.h at all, and
 * Qt's bundled SHA-3 includes it right after <endian.h>
 * (3rdparty/sha3/brg_endian.h:46), so without it qcryptographichash.cpp does
 * not get past its includes.
 *
 * Macros rather than inline functions, matching glibc: they work in C and C++
 * alike without an extern "C" wrapper, and they accept any integer expression
 * without the caller having to think about which overload applies.
 *
 * The builtins are what both GCC and clang lower these to anyway, and on
 * aarch64 they become a single rev/rev16 instruction.  The manual fallbacks
 * are here only for a compiler that lacks them; they are written as shifts and
 * masks rather than through a union or a pointer cast, so they are strict-
 * aliasing clean.
 */

#if defined(__GNUC__) || defined(__clang__)

#define bswap_16(x) __builtin_bswap16((unsigned short)(x))
#define bswap_32(x) __builtin_bswap32((unsigned int)(x))
#define bswap_64(x) __builtin_bswap64((unsigned long long)(x))

#else

#define bswap_16(x) \
    ((unsigned short)((((unsigned short)(x) & 0x00ffU) << 8) | \
                      (((unsigned short)(x) & 0xff00U) >> 8)))

#define bswap_32(x) \
    ((((unsigned int)(x) & 0x000000ffU) << 24) | \
     (((unsigned int)(x) & 0x0000ff00U) <<  8) | \
     (((unsigned int)(x) & 0x00ff0000U) >>  8) | \
     (((unsigned int)(x) & 0xff000000U) >> 24))

#define bswap_64(x) \
    ((((unsigned long long)(x) & 0x00000000000000ffULL) << 56) | \
     (((unsigned long long)(x) & 0x000000000000ff00ULL) << 40) | \
     (((unsigned long long)(x) & 0x0000000000ff0000ULL) << 24) | \
     (((unsigned long long)(x) & 0x00000000ff000000ULL) <<  8) | \
     (((unsigned long long)(x) & 0x000000ff00000000ULL) >>  8) | \
     (((unsigned long long)(x) & 0x0000ff0000000000ULL) >> 24) | \
     (((unsigned long long)(x) & 0x00ff000000000000ULL) >> 40) | \
     (((unsigned long long)(x) & 0xff00000000000000ULL) >> 56))

#endif

#endif /* EWOKOS_LIBC_BYTESWAP_H */
