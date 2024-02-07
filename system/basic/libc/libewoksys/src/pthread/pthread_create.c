#include <pthread.h>
#include <ewoksys/thread.h>
#include <ewoksys/proc.h>
#include <stddef.h>
#include <stdlib.h>

int pthread_create(pthread_t* thread,
		const pthread_attr_t *attr,
		void *(*start_routine) (void *),
		void *arg) {
	(void)attr;
	_proc_global_need_lock = true;

	pthread_t tid = thread_create(start_routine, arg);
	if(tid < 0)
		return -1;

	if(thread != NULL)
		*thread = tid;
	return 0;
}

