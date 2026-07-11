#ifndef EWOKOS_LIBC_INTTYPES_H
#define EWOKOS_LIBC_INTTYPES_H

#include <stdint.h>

typedef int64_t intmax_t;
typedef uint64_t uintmax_t;

#if __SIZEOF_LONG__ == 8
#define __EWOK_PRI64_PREFIX "l"
#else
#define __EWOK_PRI64_PREFIX "ll"
#endif

#define PRId8 "d"
#define PRIi8 "i"
#define PRIo8 "o"
#define PRIu8 "u"
#define PRIx8 "x"
#define PRIX8 "X"

#define PRId16 "d"
#define PRIi16 "i"
#define PRIo16 "o"
#define PRIu16 "u"
#define PRIx16 "x"
#define PRIX16 "X"

#define PRId32 "d"
#define PRIi32 "i"
#define PRIo32 "o"
#define PRIu32 "u"
#define PRIx32 "x"
#define PRIX32 "X"

#define PRId64 __EWOK_PRI64_PREFIX "d"
#define PRIi64 __EWOK_PRI64_PREFIX "i"
#define PRIo64 __EWOK_PRI64_PREFIX "o"
#define PRIu64 __EWOK_PRI64_PREFIX "u"
#define PRIx64 __EWOK_PRI64_PREFIX "x"
#define PRIX64 __EWOK_PRI64_PREFIX "X"

#define PRIdMAX PRId64
#define PRIiMAX PRIi64
#define PRIoMAX PRIo64
#define PRIuMAX PRIu64
#define PRIxMAX PRIx64
#define PRIXMAX PRIX64

#endif
