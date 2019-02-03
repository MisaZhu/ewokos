#include <syscall.h>
#include <kdb.h>

int kdb(void* p) {
	return syscall1(SYSCALL_KDB, (int)p);
}

