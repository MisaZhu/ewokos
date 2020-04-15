#include <kernel/uspace_int.h>
#include <kernel/proc.h>
#include <kstring.h>
#include <mm/kmalloc.h>

typedef struct {
	int32_t pid;
} uspace_interrupt_t;

static uspace_interrupt_t _uspace_int_table[USPACE_INT_MAX];

void uspace_interrupt_init(void) {
	for(int32_t i=0; i<USPACE_INT_MAX; i++) {
		_uspace_int_table[i].pid = -1;
	}
}

int32_t uspace_interrupt_register(int32_t int_id) {
	if(int_id < 0 || int_id >= USPACE_INT_MAX)
		return -1;
	if(_uspace_int_table[int_id].pid >= 0)
		return -1;
	_uspace_int_table[int_id].pid = _current_proc->pid;
	return 0;
}

int32_t uspace_interrupt_get_pid(int32_t int_id) {
	if(int_id < 0 || int_id >= USPACE_INT_MAX)
		return -1;
	return _uspace_int_table[int_id].pid;
}

void uspace_interrupt_unregister(int32_t int_id) {
	int32_t pid = _current_proc->pid;
	if(int_id < 0) { //unregister all interrupts 
		for(int32_t i=0; i<USPACE_INT_MAX; i++) {
			if(_uspace_int_table[i].pid == pid)
				_uspace_int_table[i].pid = -1;
		}
	}
	if(int_id >= USPACE_INT_MAX)
		return;
	
	if(pid == _uspace_int_table[int_id].pid)
		_uspace_int_table[int_id].pid = -1;
}

