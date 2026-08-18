#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ewoksys/sys.h>

int gethostname(char *name, size_t len) {
    sys_info_t info;
    size_t n;

    if (name == NULL || len == 0) {
        errno = EINVAL;
        return -1;
    }

    if (sys_get_sys_info(&info) != 0) {
        errno = EIO;
        return -1;
    }

    n = strlen(info.machine);
    if (n + 1 > len) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(name, info.machine, n + 1);
    return 0;
}

int sethostname(const char *name, size_t len) {
    (void)name;
    (void)len;
    errno = ENOSYS;
    return -1;
}
