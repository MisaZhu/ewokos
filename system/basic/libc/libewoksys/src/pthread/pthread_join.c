#include <pthread.h>
#include <unistd.h>
#include <ewoksys/wait.h>

int pthread_join(pthread_t thread, void **retval) {
	(void)retval;
	return waitpid(thread);
}
