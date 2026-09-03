#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

/*
 * Terminal attributes live in the backing character device and are reached
 * through ioctl(TCGETS/TCSETS...). Devices that predate termios support do
 * not answer those ioctls; for them a raw default (matching the drivers'
 * power-on state) is reported and setting is accepted without effect, so
 * existing raw-mode readers keep working.
 */
static void termios_default(struct termios *t) {
    memset(t, 0, sizeof(*t));
    t->c_iflag = 0;
    t->c_oflag = 0;
    t->c_cflag = CS8 | CREAD | CLOCAL;
    t->c_lflag = 0;         /* raw: no ICANON/ECHO/ISIG by default */
    t->c_cc[VEOF] = 4;      /* ^D */
    t->c_cc[VEOL] = '\n';
    t->c_cc[VERASE] = 0x7f;
    t->c_cc[VINTR] = 3;     /* ^C */
    t->c_cc[VKILL] = 0x15;  /* ^U */
    t->c_cc[VSTART] = 0x11; /* ^Q */
    t->c_cc[VSTOP] = 0x13;  /* ^S */
    t->c_cc[VSUSP] = 0x1a;  /* ^Z */
    t->c_cc[VMIN] = 1;
    t->c_cc[VTIME] = 0;
    t->c_ispeed = B115200;
    t->c_ospeed = B115200;
}

int tcgetattr(int fd, struct termios *termios_p) {
    if (termios_p == NULL || !isatty(fd)) {
        errno = ENOTTY;
        return -1;
    }
    if (ioctl(fd, TCGETS, termios_p) == 0)
        return 0;
    /* Driver has no termios support: report the raw default it behaves as. */
    termios_default(termios_p);
    return 0;
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p) {
    if (termios_p == NULL || !isatty(fd)) {
        errno = ENOTTY;
        return -1;
    }

    int cmd;
    switch (optional_actions) {
    case TCSANOW:  cmd = TCSETS;  break;
    case TCSADRAIN: cmd = TCSETSW; break;
    case TCSAFLUSH: cmd = TCSETSF; break;
    default:
        errno = EINVAL;
        return -1;
    }

    if (ioctl(fd, cmd, termios_p) == 0)
        return 0;
    /* Driver has no termios support: accept without effect (legacy behavior). */
    if (errno == ENOTTY)
        return 0;
    return -1;
}

int tcsendbreak(int fd, int duration) {
    (void)duration;
    if (!isatty(fd)) {
        errno = ENOTTY;
        return -1;
    }
    return 0;
}

int tcdrain(int fd) {
    if (!isatty(fd)) {
        errno = ENOTTY;
        return -1;
    }
    return 0;
}

int tcflush(int fd, int queue_selector) {
    (void)queue_selector;
    if (!isatty(fd)) {
        errno = ENOTTY;
        return -1;
    }
    return 0;
}

int tcflow(int fd, int action) {
    (void)action;
    if (!isatty(fd)) {
        errno = ENOTTY;
        return -1;
    }
    return 0;
}

void cfmakeraw(struct termios *t) {
    if (t == NULL)
        return;
    t->c_iflag &= ~(unsigned)(IGNBRK | BRKINT | PARMRK | ISTRIP |
            INLCR | IGNCR | ICRNL | IXON);
    t->c_oflag &= ~(unsigned)OPOST;
    t->c_lflag &= ~(unsigned)(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t->c_cflag &= ~(unsigned)(CSIZE | PARENB);
    t->c_cflag |= CS8;
    t->c_cc[VMIN] = 1;
    t->c_cc[VTIME] = 0;
}

speed_t cfgetispeed(const struct termios *termios_p) {
    return (termios_p != NULL) ? termios_p->c_ispeed : 0;
}

speed_t cfgetospeed(const struct termios *termios_p) {
    return (termios_p != NULL) ? termios_p->c_ospeed : 0;
}

int cfsetispeed(struct termios *termios_p, speed_t speed) {
    if (termios_p == NULL) {
        errno = EINVAL;
        return -1;
    }
    termios_p->c_ispeed = speed;
    return 0;
}

int cfsetospeed(struct termios *termios_p, speed_t speed) {
    if (termios_p == NULL) {
        errno = EINVAL;
        return -1;
    }
    termios_p->c_ospeed = speed;
    return 0;
}
