#include <pthread.h>
#include <ewoksys/thread.h>
#include <stddef.h>
#include <stdlib.h>

struct _pth_args {
	void *(*start_routine)(void *);
	void *arg;
};

/*
 * Trampoline so that TLS destructors run for threads that simply return
 * from their start routine (threads that call pthread_exit() clean up
 * there instead).
 */
static void *_pth_trampoline(void *p) {
	struct _pth_args args = *(struct _pth_args *)p;
	free(p);
	args.start_routine(args.arg);
	__pthread_tls_thread_exit();
	return NULL;
}

int pthread_create(pthread_t* thread,
        const pthread_attr_t *attr,
        void *(*start_routine) (void *),
        void *arg) {
    (void)attr;

    struct _pth_args *args = (struct _pth_args *)malloc(sizeof(struct _pth_args));
    if(args == NULL)
        return -1;
    args->start_routine = start_routine;
    args->arg = arg;

    pthread_t tid = thread_create(_pth_trampoline, args);
    if(tid < 0) {
        free(args);
        return -1;
    }

    if(thread != NULL)
        *thread = tid;
    return 0;
}
