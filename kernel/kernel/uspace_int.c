#include <kernel/uspace_int.h>
#include <kstring.h>
#include <mm/kmalloc.h>

typedef struct {
	int32_t pid;
	rawdata_t data;
} uspace_interrupt_t;

static uspace_interrupt_t _uspace_int_table[USPACE_INT_MAX];

void uspace_interrupt_init(void) {
	for(int32_t i=0; i<USPACE_INT_MAX; i++) {
		memset(&_uspace_int_table[i], 0, sizeof(uspace_interrupt_t));
		_uspace_int_table[i].pid = -1;
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
	proc_switch(ctx, int_thread, true);
	return true;
}

bool uspace_interrupt(context_t* ctx, int32_t int_id) {
	if(int_id < 0 || int_id >= USPACE_INT_MAX)
		return false;
	return proc_interrupt(ctx, _uspace_int_table[int_id].pid, int_id);
}

int32_t uspace_interrupt_register(int32_t int_id) {
	if(int_id < 0 || int_id >= USPACE_INT_MAX)
		return -1;
	if(_uspace_int_table[int_id].pid >= 0)
		return -1;
	_uspace_int_table[int_id].pid = _current_proc->pid;
	return 0;
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

static void clear_interrupt_data(int32_t int_id) {
	if(int_id < 0 || int_id >= USPACE_INT_MAX)
		return;
	if(_uspace_int_table[int_id].data.size == 0 || _uspace_int_table[int_id].data.data == NULL)
		return;

	kfree(_uspace_int_table[int_id].data.data);
	_uspace_int_table[int_id].data.data = NULL;
	_uspace_int_table[int_id].data.size = 0;
}

void uspace_set_interrupt_data(int32_t int_id, void* data, uint32_t size) {
	if(int_id < 0 || int_id >= USPACE_INT_MAX)
		return;

	clear_interrupt_data(int_id);
	if(size == 0 || data == NULL)
		return;

	_uspace_int_table[int_id].data.data = kmalloc(size);
	_uspace_int_table[int_id].data.size = size;
	memcpy(_uspace_int_table[int_id].data.data, data, size);
}

void uspace_get_interrupt_data(int32_t int_id, rawdata_t* data) {
	data->size = 0;
	data->data = NULL;

	if(int_id < 0 || int_id >= USPACE_INT_MAX)
		return;

	uint32_t sz = _uspace_int_table[int_id].data.size;
	if(sz == 0)
		return;

	data->size = sz;
	data->data = proc_malloc(sz);
	if(data->data != NULL) 
		memcpy(data->data, _uspace_int_table[int_id].data.data, sz);
	else
		data->size = 0;

	clear_interrupt_data(int_id);
}
