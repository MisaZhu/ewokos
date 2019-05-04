#include <kserv.h>
#include <kstring.h>
#include <proc.h>
#include <system.h>

#define KSERV_MAX 32

static kserv_t _kernelServices[KSERV_MAX];
static int32_t _kserv_lock = 0;

int32_t kserv_reg(const char* name) {
	if(_current_proc->owner > 0)
		return -1;
	
	CRIT_IN(_kserv_lock)
	int32_t pid = _current_proc->pid;
	int32_t i, at = -1;
	for(i=0; i<KSERV_MAX; i++) {
		if(_kernelServices[i].name[0] == 0 && at < 0) {
			at = i;
		}
		else if(strcmp(_kernelServices[i].name, name) == 0) {
			break;
		}
	}
	int32_t ret = -1;
	if(i < KSERV_MAX || at >= 0) {
		if(at < 0)
			at = i;
		if(_kernelServices[at].name[0] == 0)
			strncpy(_kernelServices[at].name, name, KSERV_NAME_MAX);
		_kernelServices[at].pid = pid;
		_kernelServices[at].state = KSERV_BUSY;
		ret = 0;
	}
	CRIT_OUT(_kserv_lock)
	return ret;
}

int32_t kserv_get_by_name(const char* name) {
	CRIT_IN(_kserv_lock)
	int32_t i, ret = -1;
	for(i=0; i<KSERV_MAX; i++) {
		if(strcmp(_kernelServices[i].name, name) == 0
				&& _kernelServices[i].state == KSERV_READY) {
			break;
		}
	}
	if(i < KSERV_MAX)
		ret = _kernelServices[i].pid;
	CRIT_OUT(_kserv_lock)
	return ret;
}


int32_t kserv_get_by_pid(int32_t pid) {
	CRIT_IN(_kserv_lock)
	int32_t i, ret = -1;
	for(i=0; i<KSERV_MAX; i++) {
		if(_kernelServices[i].pid == pid 
				&& _kernelServices[i].state == KSERV_READY) {
			break;
		}
	}
	if(i < KSERV_MAX)
		ret = _kernelServices[i].pid;
	CRIT_OUT(_kserv_lock)
	return ret;
}

int32_t kserv_ready() {
	CRIT_IN(_kserv_lock)
	int32_t pid = _current_proc->pid;
	int32_t i, ret = -1;
	for(i=0; i<KSERV_MAX; i++) {
		if(_kernelServices[i].pid == pid) {
			_kernelServices[i].state = KSERV_READY;
			break;
		}
	}
	if(i < KSERV_MAX)
		ret = 0;
	CRIT_OUT(_kserv_lock)
	return ret;
}

int32_t kserv_unreg() {
	CRIT_IN(_kserv_lock)
	process_t* proc = _current_proc;
	int32_t i, ret = -1;
	for(i=0; i<KSERV_MAX; i++) {
		if(_kernelServices[i].pid == proc->pid) {
			_kernelServices[i].name[0] = 0;
			_kernelServices[i].pid = -1;
			_kernelServices[i].state = KSERV_NONE;
			break;
		}
	}
	if(i < KSERV_MAX)
		ret = 0;
	CRIT_OUT(_kserv_lock)
	return ret;
}

