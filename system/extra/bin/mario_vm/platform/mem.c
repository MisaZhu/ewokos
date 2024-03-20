#include "mario.h"
#include "platform.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#ifdef MRCIO_THREAD
#include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MARIO_DEBUG
typedef struct mem_block {
	void* p;
	uint32_t size;
	const char* file;
	uint32_t line;
	struct mem_block *prev;
	struct mem_block *next;
} mem_block_t;

static mem_block_t* _mem_head = NULL;

#ifdef MARIO_THREAD
static pthread_mutex_t _mem_lock;
static inline void mem_lock_init() {
	pthread_mutex_init(&_mem_lock, NULL);
}
static inline void mem_lock_destroy() {
	pthread_mutex_destroy(&_mem_lock);
}
static inline void mem_lock() {
	pthread_mutex_lock(&_mem_lock);
}
static inline void mem_unlock() {
	pthread_mutex_unlock(&_mem_lock);
}

#else
static inline void mem_lock_destroy() { }
static inline void mem_lock_init() { }
static inline void mem_lock() { }
static inline void mem_unlock() { }
#endif

static inline void* raw_malloc(uint32_t size, const char* file, uint32_t line) {
	if(size == 0)
		return NULL;

	mem_lock();
	mem_block_t* block = (mem_block_t*)_platform_malloc(sizeof(mem_block_t));
	block->p = _platform_malloc(size);
	block->size = size;
	block->file = file;
	block->line = line;
	block->prev = NULL;

	if(_mem_head != NULL)
		_mem_head->prev = block;
	block->next = _mem_head;
	_mem_head = block;
	mem_unlock();
	return block->p;
}

static inline void raw_free(void* p) {
	mem_lock();
	mem_block_t* block = _mem_head;	
	while(block != NULL) {
		if(block->p == p) // found.
			break;
		block = block->next;
	}

	if(block == NULL) {
		mem_unlock();
		return;
	}
	
	if(block->next != NULL)
		block->next->prev = block->prev;
	if(block->prev != NULL)
		block->prev->next = block->next;
	
	if(block == _mem_head)
		_mem_head = block->next;

	_platform_free(block->p);
	_platform_free(block);
	mem_unlock();
}

static void raw_mem_init() { 
	_mem_head = NULL;	
	mem_lock_init();
}

static void raw_mem_quit() { 
	mem_lock();
	mem_block_t* block = _mem_head;	
	if(block != NULL) { // mem clean
		mario_debug("[debug]memory is leaking!!!\n");
		while(block != NULL) {
			mario_debug(" ");
			mario_debug(block->file);
			mario_debug(", ");
			mario_debug(mstr_from_int(block->line, 10));
			mario_debug(", size=");
			mario_debug(mstr_from_int(block->size, 10));
			mario_debug("\n");
			block = block->next;
		}
	}
	else {
		mario_debug("[debug] memory is clean.\n");
	}
	mem_unlock();
	mem_lock_destroy();
}

#else

static inline void* raw_malloc(uint32_t size) {
	return _platform_malloc(size);
}

static inline void raw_free(void* p) {
	return _platform_free(p);
}

static void raw_mem_init() { 
}

static void raw_mem_quit() { 
}
#endif


void mem_init(void) {
	_malloc = raw_malloc;
	_free = raw_free;
	raw_mem_init();
}

void mem_quit(void) {
	raw_mem_quit();
}
