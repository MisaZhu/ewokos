#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Create a unique temporary file, opened and unlinked so it is
 * removed automatically when closed. */

FILE *tmpfile(void) {
    char tmpl[] = "/tmp/ewok_tmpXXXXXX";
    int fd;

    fd = mkstemp(tmpl);
    if (fd < 0)
        return NULL;

    unlink(tmpl);
    return fdopen(fd, "w+");
}
