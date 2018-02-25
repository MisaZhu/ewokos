#include <lib/kserv.h>
#include <lib/syscall.h>
#include <lib/error.h>
#include <lib/fork.h>

int kservReg(int serv) {
	return syscall1(SYSCALL_KSERV_REG, serv);
}

int kservRequestA(int serv, char* p, int size) {
	return syscall3(SYSCALL_KSERV_REQUEST, serv, (int)p, size);
}

int kservGetResponseA(int serv, char* p, int size) {
	return syscall3(SYSCALL_KSERV_GET_RESP, serv, (int)p, size);
}

int kservGetRequestA(int serv, int *client, char* p, int size) {
	return syscall4(SYSCALL_KSERV_GET_REQUEST, serv, (int)client, (int)p, size);
}

int kservResponseA(int serv, char* p, int size) {
	return syscall3(SYSCALL_KSERV_RESP, serv, (int)p, size);
}

int kservRequest(int serv, char* p, int size) {
	while(1) {
		int ret = kservRequestA(serv, p, size);
		if(ret != E_RETRY) 
			return ret;
		yield();
	}
}

int kservGetResponse(int serv, char* p, int size) {
	while(1) {
		int ret = kservGetResponseA(serv, p, size);
		if(ret != E_RETRY) 
			return ret;
		yield();
	}
}

int kservGetRequest(int serv, int *client, char* p, int size) {
	while(1) {
		int ret = kservGetRequestA(serv, client, p, size);
		if(ret != E_RETRY) 
			return ret;
		yield();
	}
}

int kservResponse(int serv, char* p, int size) {
	while(1) {
		int ret = kservResponseA(serv, p, size);
		if(ret != E_RETRY) 
			return ret;
		yield();
	}
}


