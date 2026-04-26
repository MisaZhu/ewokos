#include <pthread.h>
#include <ewoksys/proc.h>

int pthread_detach(pthread_t thread) {
	// In ewokos, threads are child processes.
	// Detaching a thread means we don't need to wait for it.
	// We use proc_detach to detach the child process.
	(void)thread;
	// For now, just return success.
	// The actual detach is handled by not calling pthread_join.
	// When thread exits, it will be cleaned up automatically.
	return 0;
}
