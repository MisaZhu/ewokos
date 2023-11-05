#include <kernel/semaphore.h>
#include <kernel/proc.h>
#include <kstring.h>
#include <stddef.h>

#define SEMAPHORE_MAX 128

static semaphore_t _semaphores[SEMAPHORE_MAX];

void semaphore_init(void) {
	for(int32_t i=0; i<SEMAPHORE_MAX; i++) {
		_semaphores[i].occupied = SEM_IDLE;
		_semaphores[i].creater_pid = -1;
		_semaphores[i].occupied_pid = -1;
	}
}

int32_t semaphore_alloc(void) {
	for(int32_t i=0; i<SEMAPHORE_MAX; i++) {
		if(_semaphores[i].creater_pid == -1) {
			_semaphores[i].occupied = SEM_IDLE;
			return i;
		}
	}
	return -1;
}

void semaphore_free(uint32_t sem_id) {
	proc_t* cproc = get_current_proc();
	if(sem_id >= SEMAPHORE_MAX ||
			_semaphores[sem_id].creater_pid == -1 ||
			cproc == NULL)
		return;
		
	if(_semaphores[sem_id].creater_pid == cproc->info.pid) {
		_semaphores[sem_id].creater_pid = -1;
		_semaphores[sem_id].occupied = SEM_IDLE;
		_semaphores[sem_id].occupied_pid = -1;
	}	
}

int32_t semaphore_enter(context_t* ctx, uint32_t sem_id, uint8_t thread) {
	ctx->gpr[0] = -1;
	proc_t* cproc = get_current_proc();
	if(sem_id >= SEMAPHORE_MAX ||
			_semaphores[sem_id].creater_pid == -1 ||
			cproc == NULL)
		return -1;

	if(_semaphores[sem_id].occupied == SEM_OCCUPIED) {
		if(thread == 0) {
			proc_block_on(ctx, -1, sem_id, 0);
			return -1;
		}
		else { //only block all threads in same proc
			proc_t *proc = proc_get_proc(proc_get(_semaphores[sem_id].occupied_pid));
			if(proc == proc_get_proc(cproc)) {
				proc_block_on(ctx, -1, sem_id, 0);
				return -1;
			}
		}
	}

	ctx->gpr[0] = 0;
	_semaphores[sem_id].occupied = SEM_OCCUPIED;
	_semaphores[sem_id].occupied_pid = cproc->info.pid;
	return 0;
}

int32_t semaphore_quit(uint32_t sem_id) {
	proc_t* cproc = get_current_proc();
	if(sem_id >= SEMAPHORE_MAX ||
			_semaphores[sem_id].creater_pid == -1 ||
			cproc == NULL ||
			cproc->info.pid != _semaphores[sem_id].occupied_pid)
		return -1;

	_semaphores[sem_id].occupied = SEM_IDLE;
	_semaphores[sem_id].occupied_pid = -1;
	proc_wakeup(-1, sem_id, 0);
	return 0;
}