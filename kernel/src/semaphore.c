#include <semaphore.h>
#include <scheduler.h>
#include <proc.h>

int32_t semaphore_init(int32_t *s) {
	if(s == NULL)
		return -1;
	*s = 0;
	return 0;
}

int32_t semaphore_close(int32_t* s) {
	(void)s;
	return 0;
}

int32_t semaphore_lock(int32_t* s) {
	if(s == NULL || (*s) > 0)
		return -1;

	disable_schedule();
	(*s)++;
	enable_schedule();
	return 0;
}

int32_t semaphore_unlock(int32_t* s) {
	if(s == NULL)
		return -1;

	disable_schedule();
	(*s)--;
	enable_schedule();
	return 0;
}

