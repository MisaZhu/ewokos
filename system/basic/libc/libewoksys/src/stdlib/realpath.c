#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ewoksys/vfs.h>

/*
 * No symbolic links exist on this system, so realpath() only needs to
 * canonicalize "." / ".." / duplicate slashes and verify existence.
 */
char *realpath(const char *path, char *resolved) {
    char work[PATH_MAX];
    char *out;
    char *comp;
    char *p;
    size_t pos = 0;
    fsinfo_t info;

    if (path == NULL || *path == '\0') {
        errno = EINVAL;
        return NULL;
    }

    out = resolved;
    if (out == NULL) {
        out = (char *)malloc(PATH_MAX);
        if (out == NULL) {
            errno = ENOMEM;
            return NULL;
        }
    }

    /* make the path absolute */
    if (path[0] == '/') {
        work[0] = '\0';
    } else {
        if (getcwd(work, sizeof(work)) == NULL) {
            if (resolved == NULL)
                free(out);
            return NULL;
        }
    }

    p = (char *)path;
    while (*p != '\0') {
        size_t len = 0;

        while (*p == '/')
            p++;
        if (*p == '\0')
            break;
        while (p[len] != '\0' && p[len] != '/')
            len++;

        comp = p;
        p += len;

        if (len == 1 && comp[0] == '.') {
            continue;
        }
        if (len == 2 && comp[0] == '.' && comp[1] == '.') {
            /* pop one component from the accumulated path */
            if (pos > 0) {
                pos--;
                while (pos > 0 && work[pos - 1] != '/')
                    pos--;
            }
            continue;
        }
        if (pos + len + 1 >= sizeof(work)) {
            errno = ENAMETOOLONG;
            if (resolved == NULL)
                free(out);
            return NULL;
        }
        work[pos++] = '/';
        memcpy(work + pos, comp, len);
        pos += len;
        work[pos] = '\0';
    }

    if (pos == 0) {
        work[0] = '/';
        work[1] = '\0';
    }

    if (vfs_get_by_name(work, &info) != 0) {
        errno = ENOENT;
        if (resolved == NULL)
            free(out);
        return NULL;
    }

    strcpy(out, work);
    return out;
}
