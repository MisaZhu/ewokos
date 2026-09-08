#ifndef EWOKOS_LIBC_ENDIAN_H
#define EWOKOS_LIBC_ENDIAN_H

/*
 * <endian.h> - byte-order macros.
 *
 * Neither EwokOS nor newlib provides this header, but code written for glibc
 * systems includes it: Qt's bundled SHA-3 (3rdparty/sha3/brg_endian.h:44)
 * does, on any compiler that defines __GNUC__, and it was a fatal error that
 * killed qcryptographichash.cpp outright.
 *
 * Only the double-underscore spelling is defined - __BYTE_ORDER,
 * __LITTLE_ENDIAN, __BIG_ENDIAN.  That is glibc's default set (the unprefixed
 * BYTE_ORDER / LITTLE_ENDIAN / BIG_ENDIAN and the single-underscore forms are
 * behind __USE_MISC and __USE_BSD there), and it is the right choice here
 * because unprefixed LITTLE_ENDIAN and BIG_ENDIAN are plausible identifiers
 * for ordinary code to use as enum members or variable names, so defining them
 * in a header this widely included would be a namespace hazard.  Consumers
 * that probe several spellings, brg_endian.h among them, all accept this one.
 *
 * The conversion macros glibc also puts here (htobe16, le32toh and the rest)
 * are deliberately absent: nothing in the tree asks for them, and Qt has its
 * own qbswap/qFromBigEndian family.  Add them if a build ever needs them.
 *
 * Byte order is taken from the compiler rather than hardcoded.  Every EwokOS
 * port today - arm, aarch64, riscv, x86 - is little-endian, so a literal 1234
 * would work, but the toolchain already knows the answer and asking it keeps a
 * big-endian build from silently getting it wrong.
 */

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __PDP_ENDIAN    3412

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && \
    __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define __BYTE_ORDER __BIG_ENDIAN
#else
/* Covers __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, and also a compiler that
   predefines none of them, which is the case for the aarch64-none-elf
   toolchain's C mode on this platform. */
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif

#endif /* EWOKOS_LIBC_ENDIAN_H */
