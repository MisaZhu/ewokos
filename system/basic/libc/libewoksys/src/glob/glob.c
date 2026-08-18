#include <glob.h>
#include <fnmatch.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/*
 * Minimal glob(): expands patterns component by component using
 * opendir/readdir + fnmatch. Supports GLOB_APPEND, GLOB_DOOFFS,
 * GLOB_NOSORT, GLOB_MARK, GLOB_PERIOD, GLOB_NOCHECK and GLOB_ERR.
 */

static int glob_has_magic(const char *s, int flags) {
    for (const char *p = s; *p != '\0'; p++) {
        if (*p == '\\' && (flags & GLOB_NOESCAPE) == 0 && p[1] != '\0') {
            p++;
            continue;
        }
        if (*p == '*' || *p == '?' || *p == '[')
            return 1;
    }
    return 0;
}

static int glob_add(glob_t *pglob, const char *path, int flags) {
    size_t base = pglob->gl_offs;
    size_t old = pglob->gl_pathc;
    char **nv;
    char *copy;
    size_t len = strlen(path);

    copy = (char *)malloc(len + 2);
    if (copy == NULL)
        return GLOB_NOSPACE;
    memcpy(copy, path, len + 1);

    if ((flags & GLOB_MARK) != 0) {
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode) &&
                copy[len - 1] != '/') {
            copy[len] = '/';
            copy[len + 1] = '\0';
        }
    }

    nv = (char **)realloc(pglob->gl_pathv,
            (base + old + 2) * sizeof(char *));
    if (nv == NULL) {
        free(copy);
        return GLOB_NOSPACE;
    }
    pglob->gl_pathv = nv;
    pglob->gl_pathv[base + old] = copy;
    pglob->gl_pathv[base + old + 1] = NULL;
    pglob->gl_pathc = old + 1;
    return 0;
}

static void glob_sort(glob_t *pglob, size_t start) {
    size_t base = pglob->gl_offs;

    for (size_t i = start + 1; i < pglob->gl_pathc; i++) {
        char *key = pglob->gl_pathv[base + i];
        size_t j = i;
        while (j > start &&
                strcmp(pglob->gl_pathv[base + j - 1], key) > 0) {
            pglob->gl_pathv[base + j] = pglob->gl_pathv[base + j - 1];
            j--;
        }
        pglob->gl_pathv[base + j] = key;
    }
}

static void glob_join(char *out, size_t outsz, const char *dir,
        const char *name) {
    size_t n;

    if (dir[0] == '\0' || strcmp(dir, "/") == 0) {
        n = strlen(name);
        if (n + 2 > outsz)
            n = outsz - 2;
        out[0] = '/';
        memcpy(out + 1, name, n);
        out[1 + n] = '\0';
        return;
    }
    n = strlen(dir);
    if (n + 1 >= outsz)
        n = outsz - 2;
    memcpy(out, dir, n);
    if (dir[n - 1] != '/') {
        out[n++] = '/';
    }
    {
        size_t m = strlen(name);
        if (n + m + 1 > outsz)
            m = outsz - n - 1;
        memcpy(out + n, name, m);
        out[n + m] = '\0';
    }
}

static int glob_expand(const char *dir, const char *pat, int flags,
        int (*errfunc)(const char *, int), glob_t *pglob) {
    char seg[256];
    const char *slash;
    const char *rest;
    int last;
    size_t seglen;

    slash = strchr(pat, '/');
    if (slash != NULL) {
        seglen = (size_t)(slash - pat);
        rest = slash + 1;
        last = 0;
    } else {
        seglen = strlen(pat);
        rest = "";
        last = 1;
    }
    if (seglen == 0) {
        /* empty component ("a//b" or trailing slash): descend */
        if (last) {
            return (pglob->gl_pathc == 0) ? GLOB_NOMATCH : 0;
        }
        return glob_expand(dir, rest, flags, errfunc, pglob);
    }
    if (seglen >= sizeof(seg))
        return GLOB_NOMATCH;
    memcpy(seg, pat, seglen);
    seg[seglen] = '\0';

    if (!glob_has_magic(seg, flags)) {
        char full[PATH_MAX];
        struct stat st;

        glob_join(full, sizeof(full), dir, seg);
        if (stat(full, &st) != 0)
            return 0; /* literal path missing: no match here */
        if (last)
            return glob_add(pglob, full, flags);
        if (!S_ISDIR(st.st_mode))
            return 0;
        return glob_expand(full, rest, flags, errfunc, pglob);
    }

    /* magic segment: scan the directory */
    {
        DIR *d = opendir(dir[0] != '\0' ? dir : "/");
        struct dirent *e;
        int fnm_flags = FNM_PATHNAME |
                ((flags & GLOB_NOESCAPE) != 0 ? FNM_NOESCAPE : 0);

        if (d == NULL) {
            if (errfunc != NULL && errfunc(dir, errno) != 0)
                return GLOB_ABORTED;
            if ((flags & GLOB_ERR) != 0)
                return GLOB_ABORTED;
            return 0;
        }

        while ((e = readdir(d)) != NULL) {
            char full[PATH_MAX];
            int is_dir;

            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
                continue;
            if (e->d_name[0] == '.' && (flags & GLOB_PERIOD) == 0)
                continue;
            if (fnmatch(seg, e->d_name, fnm_flags) != 0)
                continue;

            glob_join(full, sizeof(full), dir, e->d_name);
            if (last) {
                int rc = glob_add(pglob, full, flags);
                if (rc != 0) {
                    closedir(d);
                    return rc;
                }
                continue;
            }

            is_dir = (e->d_type == DT_DIR);
            if (e->d_type == DT_UNKNOWN) {
                struct stat st;
                if (stat(full, &st) != 0 || !S_ISDIR(st.st_mode))
                    continue;
                is_dir = 1;
            }
            if (!is_dir)
                continue;

            {
                int rc = glob_expand(full, rest, flags, errfunc, pglob);
                if (rc == GLOB_NOSPACE || rc == GLOB_ABORTED) {
                    closedir(d);
                    return rc;
                }
            }
        }
        closedir(d);
    }
    return 0;
}

int glob(const char *pattern, int flags,
        int (*errfunc)(const char *epath, int eerrno), glob_t *pglob) {
    size_t start;
    int rc;

    if (pattern == NULL || pglob == NULL)
        return GLOB_ABORTED;

    if ((flags & GLOB_APPEND) == 0) {
        pglob->gl_pathc = 0;
        pglob->gl_pathv = NULL;
        if ((flags & GLOB_DOOFFS) == 0)
            pglob->gl_offs = 0;
        else if (pglob->gl_offs > 0) {
            pglob->gl_pathv = (char **)malloc(
                    (pglob->gl_offs + 1) * sizeof(char *));
            if (pglob->gl_pathv == NULL)
                return GLOB_NOSPACE;
            for (size_t i = 0; i <= pglob->gl_offs; i++)
                pglob->gl_pathv[i] = NULL;
        }
    }
    start = pglob->gl_pathc;

    if (pattern[0] == '/') {
        rc = glob_expand("", pattern + 1, flags, errfunc, pglob);
    } else {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
            return GLOB_ABORTED;
        rc = glob_expand(cwd, pattern, flags, errfunc, pglob);
    }

    if (rc == GLOB_NOSPACE) {
        if ((flags & GLOB_APPEND) == 0)
            globfree(pglob);
        return GLOB_NOSPACE;
    }
    if (rc == GLOB_ABORTED) {
        if ((flags & GLOB_APPEND) == 0)
            globfree(pglob);
        return GLOB_ABORTED;
    }

    if (pglob->gl_pathc == start) {
        if ((flags & GLOB_NOCHECK) != 0) {
            rc = glob_add(pglob, pattern, flags & ~GLOB_MARK);
            if (rc != 0) {
                if ((flags & GLOB_APPEND) == 0)
                    globfree(pglob);
                return rc;
            }
        } else {
            return GLOB_NOMATCH;
        }
    }

    if ((flags & GLOB_NOSORT) == 0)
        glob_sort(pglob, start);
    return 0;
}

void globfree(glob_t *pglob) {
    if (pglob == NULL)
        return;
    if (pglob->gl_pathv != NULL) {
        size_t base = pglob->gl_offs;
        for (size_t i = 0; i < pglob->gl_pathc; i++) {
            free(pglob->gl_pathv[base + i]);
        }
        free(pglob->gl_pathv);
    }
    pglob->gl_pathc = 0;
    pglob->gl_pathv = NULL;
}
