#include <pthread.h>
#include <ewoksys/syscall.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

/*
 * Thread-specific data. EwokOS threads are child tasks with distinct ids,
 * so per-thread values are kept in a list keyed by thread id and protected
 * by a simple spin lock.
 */
typedef struct tls_entry {
	int32_t tid;
	void *values[PTHREAD_KEYS_MAX];
	struct tls_entry *next;
} tls_entry_t;

static struct {
	int in_use;
	void (*destructor)(void *);
} _keys[PTHREAD_KEYS_MAX];

static tls_entry_t *_tls_list = NULL;

static volatile int _tls_locked = 0;

static void tls_lock(void) {
	while(__sync_lock_test_and_set(&_tls_locked, 1) != 0)
		syscall0(SYS_YIELD);
}

static void tls_unlock(void) {
	__sync_lock_release(&_tls_locked);
}

static tls_entry_t *tls_find_locked(int32_t tid) {
	tls_entry_t *e = _tls_list;
	while(e != NULL) {
		if(e->tid == tid)
			return e;
		e = e->next;
	}
	return NULL;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
	if(key == NULL)
		return EINVAL;

	tls_lock();
	for(int i = 0; i < PTHREAD_KEYS_MAX; i++) {
		if(!_keys[i].in_use) {
			_keys[i].in_use = 1;
			_keys[i].destructor = destructor;
			*key = i;
			tls_unlock();
			return 0;
		}
	}
	tls_unlock();
	return EAGAIN;
}

int pthread_key_delete(pthread_key_t key) {
	if(key < 0 || key >= PTHREAD_KEYS_MAX)
		return EINVAL;

	tls_lock();
	if(!_keys[key].in_use) {
		tls_unlock();
		return EINVAL;
	}
	_keys[key].in_use = 0;
	_keys[key].destructor = NULL;
	tls_unlock();
	return 0;
}

int pthread_setspecific(pthread_key_t key, const void *value) {
	if(key < 0 || key >= PTHREAD_KEYS_MAX)
		return EINVAL;

	tls_lock();
	if(!_keys[key].in_use) {
		tls_unlock();
		return EINVAL;
	}

	int32_t tid = (int32_t)pthread_self();
	tls_entry_t *e = tls_find_locked(tid);
	if(e == NULL) {
		tls_unlock();
		e = (tls_entry_t *)malloc(sizeof(tls_entry_t));
		if(e == NULL)
			return ENOMEM;
		memset(e, 0, sizeof(tls_entry_t));
		e->tid = tid;
		tls_lock();
		e->next = _tls_list;
		_tls_list = e;
	}
	e->values[key] = (void *)value;
	tls_unlock();
	return 0;
}

void *pthread_getspecific(pthread_key_t key) {
	void *value = NULL;

	if(key < 0 || key >= PTHREAD_KEYS_MAX)
		return NULL;

	tls_lock();
	if(_keys[key].in_use) {
		tls_entry_t *e = tls_find_locked((int32_t)pthread_self());
		if(e != NULL)
			value = e->values[key];
	}
	tls_unlock();
	return value;
}

/*
 * Run registered destructors and drop the current thread's value storage.
 * Destructors are invoked outside the lock so they may use TLS themselves.
 */
void __pthread_tls_thread_exit(void) {
	int32_t tid = (int32_t)pthread_self();

	for(int iter = 0; iter < PTHREAD_DESTRUCTOR_ITERATIONS; iter++) {
		void (*destructors[PTHREAD_KEYS_MAX])(void *);
		void *values[PTHREAD_KEYS_MAX];
		int count = 0;

		tls_lock();
		tls_entry_t *e = tls_find_locked(tid);
		if(e != NULL) {
			for(int i = 0; i < PTHREAD_KEYS_MAX; i++) {
				if(_keys[i].in_use && _keys[i].destructor != NULL &&
						e->values[i] != NULL) {
					destructors[count] = _keys[i].destructor;
					values[count] = e->values[i];
					e->values[i] = NULL;
					count++;
				}
			}
		}
		tls_unlock();

		if(count == 0)
			break;
		for(int i = 0; i < count; i++)
			destructors[i](values[i]);
	}

	tls_lock();
	tls_entry_t **link = &_tls_list;
	while(*link != NULL) {
		if((*link)->tid == tid) {
			tls_entry_t *e = *link;
			*link = e->next;
			tls_unlock();
			free(e);
			return;
		}
		link = &(*link)->next;
	}
	tls_unlock();
}
