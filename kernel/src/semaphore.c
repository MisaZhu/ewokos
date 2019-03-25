#include <semaphore.h>
#include <scheduler.h>
#include <proc.h>

typedef struct {
	int32_t value;
	int32_t ring;
	int32_t owner;
} semaphore_t;

#define SEMAPHORE_MAX 128
static semaphore_t _semaphores[SEMAPHORE_MAX];

void semaphore_init() {
	int32_t i;
	for(i=0; i<SEMAPHORE_MAX; i++) {
		_semaphores[i].value = 0;
		_semaphores[i].ring = -1;
		_semaphores[i].owner = -1;
	}
}

int32_t semaphore_alloc() {
	int32_t i;
	for(i=0; i<SEMAPHORE_MAX; i++) {
		if(_semaphores[i].owner < 0) {
			_semaphores[i].owner = _current_proc->pid;
			return i;
		}
	}
	return -1;
}

void semaphore_free(int32_t s) {
	if(s < 0 || s > SEMAPHORE_MAX ||
			_semaphores[s].owner != _current_proc->pid)
		return;
	_semaphores[s].owner = -1;
	_semaphores[s].ring = -1;
	_semaphores[s].value = 0;
}

int32_t semaphore_lock(int32_t s) {
	if(s < 0 || s > SEMAPHORE_MAX ||
			_semaphores[s].owner < 0 ||
			_semaphores[s].value != 0)
		return -1;

	disable_schedule();
	_semaphores[s].ring = _current_proc->pid;
	_semaphores[s].value++;
	enable_schedule();
	return 0;
}

int32_t semaphore_unlock(int32_t s) {
	if(s < 0 || s > SEMAPHORE_MAX ||
			_semaphores[s].owner < 0 ||
			_semaphores[s].ring != _current_proc->pid ||
			_semaphores[s].value <= 0)
		return -1;

	disable_schedule();
	_semaphores[s].value--;
	enable_schedule();
	return 0;
}

