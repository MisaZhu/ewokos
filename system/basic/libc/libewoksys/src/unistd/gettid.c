#include <unistd.h>
#include <ewoksys/thread.h>

int gettid(void) {
	return thread_get_id();
}

