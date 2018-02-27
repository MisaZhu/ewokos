#include <serv.h>
#include <syscall.h>
#include <error.h>
#include <fork.h>

int servReg(int serv) {
	return syscall1(SYSCALL_KSERV_REG, serv);
}

int servRequestA(int serv, char* p, int size) {
	return syscall3(SYSCALL_KSERV_REQUEST, serv, (int)p, size);
}

int servGetResponseA(int serv, char* p, int size) {
	return syscall3(SYSCALL_KSERV_GET_RESP, serv, (int)p, size);
}

int servGetRequestA(int serv, int *client, char* p, int size) {
	return syscall4(SYSCALL_KSERV_GET_REQUEST, serv, (int)client, (int)p, size);
}

int servResponseA(int serv, char* p, int size) {
	return syscall3(SYSCALL_KSERV_RESP, serv, (int)p, size);
}

int servRequest(int serv, char* p, int size) {
	while(1) {
		int ret = servRequestA(serv, p, size);
		if(ret != E_RETRY) 
			return ret;
		yield();
	}
}

int servGetResponse(int serv, char* p, int size) {
	while(1) {
		int ret = servGetResponseA(serv, p, size);
		if(ret != E_RETRY) 
			return ret;
		yield();
	}
}

int servGetRequest(int serv, int *client, char* p, int size) {
	while(1) {
		int ret = servGetRequestA(serv, client, p, size);
		if(ret != E_RETRY) 
			return ret;
		yield();
	}
}

int servResponse(int serv, char* p, int size) {
	while(1) {
		int ret = servResponseA(serv, p, size);
		if(ret != E_RETRY) 
			return ret;
		yield();
	}
}


