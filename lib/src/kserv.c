#include <lib/kserv.h>
#include <lib/syscall.h>

int kservReg(int serv) {
	return syscall1(SYSCALL_KSERV_REG, serv);
}

int kservWrite(int serv, char* p, int size) {
	return syscall3(SYSCALL_KSERV_WRITE, serv, (int)p, size);
}

int kservRead(int serv, char* p, int size) {
	return syscall3(SYSCALL_KSERV_READ, serv, (int)p, size);
}


