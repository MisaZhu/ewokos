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
		ret = 0;
	}
	CRIT_OUT(_kserv_lock)
	return ret;
}

int32_t kserv_get(const char* name) {
	CRIT_IN(_kserv_lock)
	int32_t i, ret = -1;
	for(i=0; i<KSERV_MAX; i++) {
		if(strcmp(_kernelServices[i].name, name) == 0) {
			break;
		}
	}
	if(i < KSERV_MAX)
		ret = _kernelServices[i].pid;
	CRIT_OUT(_kserv_lock)
	return ret;
}

int32_t kserv_unreg(process_t* proc) {
	CRIT_IN(_kserv_lock)
	int32_t i, ret = -1;
	for(i=0; i<KSERV_MAX; i++) {
		if(_kernelServices[i].pid == proc->pid) {
			_kernelServices[i].name[0] = 0;
			_kernelServices[i].pid = -1;
			break;
		}
	}
	if(i < KSERV_MAX)
		ret = 0;
	CRIT_OUT(_kserv_lock)
	return ret;
}

