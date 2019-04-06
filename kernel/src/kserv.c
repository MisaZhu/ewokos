#include <kserv.h>
#include <kstring.h>
#include <proc.h>
//#include <stdio.h>

#define KSERV_MAX 32

static kserv_t _kernelServices[KSERV_MAX];

int kserv_reg(const char* name) {
	if(_current_proc->owner > 0) {
		return -1;
	}	

	int pid = _current_proc->pid;
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

int kserv_get(const char* name) {
	int i;
	for(i=0; i<KSERV_MAX; i++) {
		if(strcmp(_kernelServices[i].name, name) == 0) {
			return _kernelServices[i].pid;
		}
	}
	return -1;
}

int kserv_unreg(process_t* proc) {
	int i;
	for(i=0; i<KSERV_MAX; i++) {
		if(_kernelServices[i].pid == proc->pid) {
			_kernelServices[i].name[0] = 0;
			_kernelServices[i].pid = -1;
			return 0;
		}
	}
	return -1;
}

