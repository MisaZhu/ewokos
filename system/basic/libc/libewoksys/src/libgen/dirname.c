#include <libgen.h>
#include <string.h>

/* XSI dirname(): may modify its argument, result lives in a static buffer. */
char *dirname(char *path) {
    static char dot[] = ".";
    size_t len;
    char *p;

    if (path == NULL || *path == '\0')
        return dot;

    len = strlen(path);
    /* strip trailing slashes */
    while (len > 1 && path[len - 1] == '/') {
        path[--len] = '\0';
    }

    p = strrchr(path, '/');
    if (p == NULL)
        return dot;
    if (p == path)
        return (char *)"/";

    *p = '\0';
    /* strip remaining trailing slashes */
    p--;
    while (p > path && *p == '/') {
        *p-- = '\0';
    }
    return path;
}
