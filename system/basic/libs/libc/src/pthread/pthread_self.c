#include <pthread.h>
#include <ewoksys/thread.h>

pthread_t pthread_self(void) {
	return thread_get_id();
}
