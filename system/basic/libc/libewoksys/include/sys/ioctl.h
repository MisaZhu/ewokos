#ifndef EWOKOS_LIBC_SYS_IOCTL_H
#define EWOKOS_LIBC_SYS_IOCTL_H

#define FIOCLEX  0x5451
#define FIONCLEX 0x5450
#define FIONBIO  0x5421
#define FIONREAD 0x541B

/* Terminal ioctls. Values follow the Linux ABI so ported code keeps working;
 * they are routed to the backing character device via dev_cntl. */
#define TCGETS     0x5401
#define TCSETS     0x5402
#define TCSETSW    0x5403
#define TCSETSF    0x5404
#define TIOCGWINSZ 0x5413
#define TIOCSWINSZ 0x5414

/* Window size reported/set through TIOCGWINSZ / TIOCSWINSZ. */
struct winsize {
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;
	unsigned short ws_ypixel;
};

#define ioctlsocket ioctl

#endif
