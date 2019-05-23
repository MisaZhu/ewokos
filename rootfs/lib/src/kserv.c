#include "kserv.h"
#include <ipc.h>
#include <vfs/fs.h>
#include <syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int kserv_register(const char* reg_name) {
	return syscall1(SYSCALL_KSERV_REG, (int)reg_name);
}

static void do_call(package_t* pkg, kserv_call_func_t call_func, void* pcall) {
	proto_t* in = proto_new(get_pkg_data(pkg), pkg->size);
	proto_t* out = proto_new(NULL, 0);
	int32_t res = call_func(pkg->pid, pkg->type, in, out, pcall);	

	if(errno == EAGAIN) {
		ipc_send(pkg->id, PKG_TYPE_AGAIN, NULL, 0);
		errno = ENONE;
	}
	else if(res < 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
	}
	else {
		ipc_send(pkg->id, pkg->type, out->data, out->size);
	}
	proto_free(in);
	proto_free(out);
}

int kserv_run(kserv_call_func_t call_func, void* pcall,
		kserv_step_func_t step_func, void* pstep) {
	bool block = step_func == NULL ? true:false;
  while(true) {
    package_t* pkg = ipc_roll(block);
    if(pkg != NULL) {
			if(call_func != NULL) {
				do_call(pkg, call_func, pcall);
			}
      free(pkg);
    }
    else {
			if(step_func != NULL)
				step_func(pstep);
	    sleep(0);
		}
  }
	return 0;
}

int kserv_ready() {
	return syscall0(SYSCALL_KSERV_READY);
}

int kserv_get_by_name(const char* reg_name) {
	return syscall1(SYSCALL_KSERV_GET_BY_NAME, (int)reg_name);
}

int kserv_get_by_pid(int32_t pid) {
	return syscall1(SYSCALL_KSERV_GET_BY_PID, pid);
}

void kserv_wait_by_name(const char* reg_name) {
	while(kserv_get_by_name(reg_name) < 0)
		sleep(0);
}

void kserv_wait_by_pid(int32_t pid) {
	while(kserv_get_by_pid(pid) < 0)
		sleep(0);
}
