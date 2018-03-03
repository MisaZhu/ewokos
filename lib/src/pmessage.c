#include <pmessage.h>
#include <syscall.h>

int psendMessage(int toPid, void* data, int size) {
	return syscall3(SYSCALL_SEND_MSG, toPid, (int)data, size);
}

ProcMessageT* preadMessage(int fromPid) {
	return (ProcMessageT*)syscall1(SYSCALL_READ_MSG, fromPid);
}


