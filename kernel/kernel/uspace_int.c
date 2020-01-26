#include <kernel/uspace_int.h>
#include <kstring.h>

static int32_t _uspace_int_table[USPACE_INT_MAX];

void uspace_interrupt_init(void) {
	for(int32_t i=0; i<USPACE_INT_MAX; i++) {
		_uspace_int_table[i] = -1;
	}
}

bool proc_interrupt(context_t* ctx, int32_t pid, int32_t int_id) {
	if(pid < 0)
		return false;

	proc_t* proc = proc_get(pid);	
	if(proc == NULL || proc->interrupt.entry == 0 || proc->interrupt.busy)
		return false;

	proc_t *int_thread = kfork_raw(PROC_TYPE_INTERRUPT, proc);
	if(int_thread == NULL)
		return false;

	uint32_t sp = int_thread->ctx.sp;
	memcpy(&int_thread->ctx, &proc->ctx, sizeof(context_t));
	int_thread->ctx.sp = sp;
	int_thread->ctx.pc = int_thread->ctx.lr = proc->interrupt.entry;
	int_thread->ctx.gpr[0] = int_id;
	int_thread->ctx.gpr[1] = proc->interrupt.func;
	int_thread->ctx.gpr[2] = proc->interrupt.data;
	proc->interrupt.busy = true;
	proc_switch(ctx, int_thread);
	return true;
}

bool uspace_interrupt(context_t* ctx, int32_t int_id) {
	if(int_id < 0 || int_id >= USPACE_INT_MAX)
		return false;
	return proc_interrupt(ctx, _uspace_int_table[int_id], int_id);
}

int32_t uspace_interrupt_register(int32_t int_id) {
	if(int_id < 0 || int_id >= USPACE_INT_MAX)
		return -1;
	if(_uspace_int_table[int_id] >= 0)
		return -1;
	_uspace_int_table[int_id] = _current_proc->pid;
	return 0;
}

void uspace_interrupt_unregister(int32_t int_id) {
	int32_t pid = _current_proc->pid;
	if(int_id < 0) { //unregister all interrupts 
		for(int32_t i=0; i<USPACE_INT_MAX; i++) {
			if(_uspace_int_table[i] == pid)
				_uspace_int_table[i] = -1;
		}
	}
	if(int_id >= USPACE_INT_MAX)
		return;
	
	if(pid == _uspace_int_table[int_id])
		_uspace_int_table[int_id] = -1;
}
