#ifndef SYSCALL_H
#define SYSCALL_H

int syscall0(int code);
int syscall1(int code, int arg0);
int syscall2(int code, int arg0, int arg1);
int syscall3(int code, int arg0, int arg1, int arg2);

typedef enum {
	SYSCALL_PUTCH,
	SYSCALL_FORK,
	SYSCALL_WAIT,
	SYSCALL_YIELD,
	SYSCALL_EXIT,
	SYSCALL_PMALLOC,
	SYSCALL_PFREE,

	SYSCALL_KSERV_REG,
	SYSCALL_KSERV_WRITE,
	SYSCALL_KSERV_READ,
} SyscallCodeT;


#endif
