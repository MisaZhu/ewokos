#include "kserv.h"
#include <ipc.h>
#include <syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

bool kserv_run(const char* reg_name, kserv_func_t servFunc, void* p) {
	if(syscall1(SYSCALL_KSERV_REG, (int)reg_name) != 0) {
		return false;
	}

  while(true) {
    package_t* pkg = ipc_roll();
    if(pkg != NULL) {
      servFunc(pkg, p);
      free(pkg);
    }
    else
      yield();
  }
	return true;
}

int kserv_get_pid(const char* reg_name) {
	return syscall1(SYSCALL_KSERV_GET, (int)reg_name);
}

void kserv_wait(const char* reg_name) {
	while(kserv_get_pid(reg_name) < 0)
		yield();
}
