#include <pthread.h>
#include <sys/thread.h>
#include <stddef.h>

void pthread_create(pthread_t* thread, const pthread_attr_t *attr,
		void *(*start_routine) (void *), void *arg) {
	(void)attr;
	pthread_t tid = thread_create(start_routine, arg);
	if(thread != NULL)
		*thread = tid;
}
