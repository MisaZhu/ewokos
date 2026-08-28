#include <stdio.h>
#include <sys/types.h>

/* off_t variants of ftell/fseek (POSIX) */

off_t ftello(FILE *stream) {
    return (off_t)ftell(stream);
}

int fseeko(FILE *stream, off_t offset, int whence) {
    return fseek(stream, (long)offset, whence);
}
