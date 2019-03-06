#include <fb.h>
#include <syscall.h>

int fbGetInfo(FBInfoT* info) {
	return syscall1(SYSCALL_FB_INFO, (int32_t)info);
}

int fbOpen() {
	return syscall0(SYSCALL_FB_OPEN);
}

int fbClose() {
	return syscall0(SYSCALL_FB_CLOSE);
}

int fbWrite(void* data, uint32_t size) {
	return syscall2(SYSCALL_FB_WRITE, (int32_t)data, (int32_t)size);
}


