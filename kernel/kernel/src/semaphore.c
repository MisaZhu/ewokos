#include <kernel/semaphore.h>
#include <kernel/proc.h>
#include <kstring.h>
#include <stddef.h>

#define SEMAPHORE_MAX 1024

static semaphore_t _semaphores[SEMAPHORE_MAX];

void semaphore_init(void) {
	for(int32_t i=0; i<SEMAPHORE_MAX; i++) {
		_semaphores[i].occupied = SEM_IDLE;
		_semaphores[i].creater_pid = -1;
		_semaphores[i].occupied_pid = -1;
	}
}

int32_t semaphore_alloc(void) {
	proc_t* proc = proc_get_proc(get_current_proc());
	if(proc == NULL)
		return 0;

	for(int32_t i=0; i<SEMAPHORE_MAX; i++) {
		if(_semaphores[i].creater_pid == -1) {
			_semaphores[i].creater_pid = proc->info.pid;
			_semaphores[i].occupied = SEM_IDLE;
			_semaphores[i].occupied_pid = -1;
			return i+1;
		}
	}
	return 0;
}

void semaphore_clear(int32_t pid) {
	for(int32_t i=0; i<SEMAPHORE_MAX; i++) {
		if(_semaphores[i].creater_pid == pid) {
			semaphore_quit(i+1);
			_semaphores[i].occupied = SEM_IDLE;
			_semaphores[i].creater_pid = -1;
			_semaphores[i].occupied_pid = -1;
		}
	}
}

void semaphore_free(uint32_t sem_id) {
	if(sem_id == 0)
		return;
	sem_id--;

	proc_t* cproc = proc_get_proc(get_current_proc());
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

int32_t semaphore_enter(context_t* ctx, uint32_t sem_id) {
	if(sem_id == 0)
		return -1;
	sem_id--;

	proc_t* cproc = get_current_proc();
	if(sem_id >= SEMAPHORE_MAX ||
			_semaphores[sem_id].creater_pid == -1 ||
			cproc == NULL)
		return -1;

	if(_semaphores[sem_id].occupied == SEM_OCCUPIED &&
			_semaphores[sem_id].occupied_pid != cproc->info.pid) {
		ctx->gpr[0] = -2;
		proc_block_on(ctx, _semaphores[sem_id].occupied_pid, (uint32_t)_semaphores + sem_id);
		return -2;
	}

	_semaphores[sem_id].occupied = SEM_OCCUPIED;
	_semaphores[sem_id].occupied_pid = cproc->info.pid;
	return 0;
}

int32_t semaphore_quit(uint32_t sem_id) {
	if(sem_id == 0)
		return -1;
	sem_id--;

	proc_t* cproc = get_current_proc();

	if(sem_id >= SEMAPHORE_MAX ||
			_semaphores[sem_id].creater_pid == -1 ||
			_semaphores[sem_id].occupied_pid == -1 ||
			cproc == NULL ||
			cproc->info.pid != _semaphores[sem_id].occupied_pid)
		return -1;

	int pid_by = _semaphores[sem_id].occupied_pid;
	_semaphores[sem_id].occupied = SEM_IDLE;
	_semaphores[sem_id].occupied_pid = -1;
	proc_wakeup(pid_by, -1, (uint32_t)_semaphores + sem_id);
	return 0;
}