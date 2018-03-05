#ifndef SYSCALL_H
#define SYSCALL_H

int syscall0(int code);
int syscall1(int code, int arg0);
int syscall2(int code, int arg0, int arg1);
int syscall3(int code, int arg0, int arg1, int arg2);

typedef enum {
	SYSCALL_UART_PUTCH,
	SYSCALL_UART_GETCH,

	SYSCALL_FORK,
	SYSCALL_GETPID,
	SYSCALL_EXEC,
	SYSCALL_EXEC_ELF,
	SYSCALL_WAIT,
	SYSCALL_YIELD,
	SYSCALL_EXIT,

	SYSCALL_PMALLOC,
	SYSCALL_PFREE,

	SYSCALL_SEND_MSG,
	SYSCALL_READ_MSG,

	SYSCALL_READ_INITRD,
} SyscallCodeT;


#endif
