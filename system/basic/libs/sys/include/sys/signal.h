#ifndef SYS_SIGNAL_H
#define SYS_SIGNAL_H

enum {
	SYS_SIGNAL_EXIT = 0,
	SYS_SIGNAL_NUM
};

typedef void(*signal_handler_t)(int signo);

void sys_singal_init(void);
int  sys_singal(int sig_no, signal_handler_t handler);

#endif
