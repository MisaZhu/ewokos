#include <lib/syscall.h>
#include <syscalls.h>
#include <types.h>
#include <console.h>

static int syscall_putch(int c)
{
	kputch(c);
	return 0;
}

static int (*const _syscallHandler[])() = {
	[SYSCALL_PUTCH] = syscall_putch
};

/* kernel side of system calls. */
int handleSyscall(int code, int arg1, int arg2, int arg3)
{
	return _syscallHandler[code](arg1, arg2, arg3);
}

