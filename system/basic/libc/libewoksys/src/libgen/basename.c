#include <libgen.h>
#include <string.h>

/* XSI basename(): may modify its argument, result lives in a static buffer. */
char *basename(char *path) {
    static char dot[] = ".";
    char *p;
    size_t len;

    if (path == NULL || *path == '\0')
        return dot;

    /* strip trailing slashes */
    len = strlen(path);
    while (len > 1 && path[len - 1] == '/') {
        path[--len] = '\0';
    }
    if (len == 0)
        return (char *)"/";

    p = strrchr(path, '/');
    if (p == NULL)
        return path;
    if (p[1] == '\0')
        return (char *)"/";
    return p + 1;
}
