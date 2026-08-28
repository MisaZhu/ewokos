#include <unistd.h>
#include <stdlib.h>
#include <string.h>

extern char **environ;
extern int _execve(const char *name, char *const argv[], char *const env[]);

int execve(const char *pathname, char *const argv[], char *const envp[]) {
    return _execve(pathname, argv, envp);
}

int execv(const char *pathname, char *const argv[]) {
    return _execve(pathname, argv, environ);
}

int execvp(const char *file, char *const argv[]) {
    char path[512];
    const char *paths;
    const char *p;
    size_t len;

    if (file == NULL || file[0] == 0)
        return -1;

    /* a path separator means the file is taken as-is */
    if (strchr(file, '/') != NULL)
        return execv(file, argv);

    paths = getenv("PATH");
    if (paths == NULL)
        paths = "/bin:/sbin";

    len = strlen(file);
    p = paths;
    while (*p != 0) {
        const char *end = strchr(p, ':');
        size_t n = (end != NULL) ? (size_t)(end - p) : strlen(p);

        if (n > 0 && n + len + 2 <= sizeof(path)) {
            memcpy(path, p, n);
            path[n] = '/';
            memcpy(path + n + 1, file, len + 1);
            execv(path, argv);
        }
        if (end == NULL)
            break;
        p = end + 1;
    }
    return -1;
}
