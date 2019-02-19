#include "kserv/kserv.h"
#include <pmessage.h>
#include <syscall.h>
#include <unistd.h>
#include <stdlib.h>

bool kservRun(const char* regName, KServFuncT servFunc, void* p) {
	if(syscall1(SYSCALL_KSERV_REG, (int)regName) != 0) {
		return false;
	}

  while(true) {
    PackageT* pkg = proll();
    if(pkg != NULL) {
      servFunc(pkg, p);
      free(pkg);
    }
    else
      yield();
  }
	return true;
}

int kservGetPid(const char* regName) {
	return syscall1(SYSCALL_KSERV_GET, (int)regName);
}
