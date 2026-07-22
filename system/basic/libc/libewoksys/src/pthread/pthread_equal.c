#include <pthread.h>

int pthread_equal(pthread_t t1, pthread_t t2) {
	return t1 == t2;
}
