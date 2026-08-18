#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ewoksys/vfs.h>

static char ttyname_buf[64];

int ttyname_r(int fd, char *buf, size_t buflen) {
    fsinfo_t info;

    if (!isatty(fd))
        return ENOTTY;
    if (vfs_get_by_fd(fd, &info) != 0)
        return ENOTTY;

    if (strlen("/dev/") + strlen(info.name) + 1 > buflen)
        return ERANGE;

    strcpy(buf, "/dev/");
    strcat(buf, info.name);
    return 0;
}

char *ttyname(int fd) {
    if (ttyname_r(fd, ttyname_buf, sizeof(ttyname_buf)) != 0)
        return NULL;
    return ttyname_buf;
}

char *ctermid(char *s) {
    static char ctermid_buf[L_ctermid];

    if (s == NULL)
        s = ctermid_buf;
    strcpy(s, "/dev/tty0");
    return s;
}
