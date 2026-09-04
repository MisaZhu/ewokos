#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>

/*
 * Route a terminal request to the character device backing fd. The payload
 * (struct termios or struct winsize) travels as a single blob through
 * dev_cntl; when write_arg is false the driver's reply blob is copied back
 * into argp. Returns 0 on success, otherwise sets errno (ENOTTY when fd is
 * not a terminal or the driver does not implement the request) and returns -1.
 */
static int tty_dev_ioctl(int fd, int cmd, void* argp, uint32_t size, bool write_arg) {
    if(argp == NULL) {
        errno = EINVAL;
        return -1;
    }

    fsinfo_t info;
    if(vfs_get_by_fd(fd, &info) != 0) {
        errno = EBADF;
        return -1;
    }
    if(info.mount_pid <= 0 || !FS_IS_TYPE(info.type, FS_TYPE_CHAR)) {
        errno = ENOTTY;
        return -1;
    }

    proto_t in, out;
    PF->init(&in);
    PF->init(&out);
    if(write_arg)
        PF->add(&in, argp, size);

    int res = dev_cntl_by_pid(info.mount_pid, cmd, &in, &out);
    if(res == 0 && !write_arg) {
        int32_t sz = 0;
        void* data = proto_read(&out, &sz);
        if(data != NULL && (uint32_t)sz == size)
            memcpy(argp, data, size);
        else
            res = -1;
    }

    PF->clear(&in);
    PF->clear(&out);

    if(res != 0) {
        errno = ENOTTY;
        return -1;
    }
    return 0;
}

int ioctl(int fd, int cmd, ...) {
    va_list args;
    void *argp;

    va_start(args, cmd);
    argp = va_arg(args, void *);
    va_end(args);

    if (cmd == FIONBIO) {
        int flags;
        int enabled;

        if (argp == NULL) {
            errno = EINVAL;
            return -1;
        }

        flags = fcntl(fd, F_GETFL);
        if (flags < 0) {
            return -1;
        }
        enabled = (*(int *)argp != 0);
        if (enabled) {
            flags |= O_NONBLOCK;
        }
        else {
            flags &= ~O_NONBLOCK;
        }
        return fcntl(fd, F_SETFL, flags);
    }

    if (cmd == FIOCLEX || cmd == FIONCLEX) {
        int flags = fcntl(fd, F_GETFD);
        if (flags < 0) {
            return -1;
        }
        if (cmd == FIOCLEX) {
            flags |= FD_CLOEXEC;
        }
        else {
            flags &= ~FD_CLOEXEC;
        }
        return fcntl(fd, F_SETFD, flags);
    }

    switch(cmd) {
    case TCGETS:
        return tty_dev_ioctl(fd, cmd, argp, sizeof(struct termios), false);
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
        return tty_dev_ioctl(fd, cmd, argp, sizeof(struct termios), true);
    case TIOCGWINSZ:
        return tty_dev_ioctl(fd, cmd, argp, sizeof(struct winsize), false);
    case TIOCSWINSZ:
        return tty_dev_ioctl(fd, cmd, argp, sizeof(struct winsize), true);
    default:
        break;
    }

    errno = ENOTTY;
    return -1;
}
