#include <kserv.h>
#include <kstring.h>
#include <proc.h>
//#include <stdio.h>

#define KSERV_MAX 32

static KServT _kernelServices[KSERV_MAX];

int kservReg(const char* name) {
	int pid = _currentProcess->pid;
	int i;
	for(i=0; i<KSERV_MAX; i++) {
		if(_kernelServices[i].name[0] == 0) 
			break;

		if(strcmp(_kernelServices[i].name, name) == 0) {
			_kernelServices[i].pid = pid;
			return 0;
		}
	}
	
	if(i >= KSERV_MAX) {
		return -1;
	}

	strncpy(_kernelServices[i].name, name, KSERV_NAME_MAX);
	_kernelServices[i].pid = pid;
	return 0;
}

int kservGet(const char* name) {
	int i;
	for(i=0; i<KSERV_MAX; i++) {
		if(_kernelServices[i].name[0] == 0) 
			break;

		if(strcmp(_kernelServices[i].name, name) == 0) {
			return _kernelServices[i].pid;
		}
	}
	return -1;
}


