#include <stdlib.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <ewoksys/trunkmem.h>
#include <pthread.h>

#define MALLOC_BUF_SIZE_DEF  (4096)
#define MALLOC_SEG_SIZE_DEF  (128)

static malloc_t __malloc_info__;
static char* _malloc_buf;
static uint32_t _malloc_mem_tail;
static uint32_t _malloc_buf_size;
static uint32_t _malloc_seg_size;

static void m_shrink(void* arg, int32_t pages) {
	(void)arg;
	_malloc_mem_tail -= pages * __malloc_info__.seg_size;
}

static int32_t m_expand(void* arg, int32_t pages) {
	(void)arg;
	uint32_t to = _malloc_mem_tail + (pages * __malloc_info__.seg_size);
	uint32_t tail = (uint32_t)(_malloc_buf) + _malloc_buf_size;
	if(to > tail) {
		uint32_t sz = to - tail;
		if(sz < MALLOC_BUF_SIZE_DEF)
			sz = MALLOC_BUF_SIZE_DEF;
		_malloc_buf = proc_malloc_expand(sz);
		if(_malloc_buf == NULL)
			return -1;
		_malloc_buf_size = proc_malloc_size();
	}

	_malloc_mem_tail = to;
	return 0;
}

static void* m_get_mem_tail(void* arg) {
	(void)arg;
	return (void*)_malloc_mem_tail;
}

void __ewok_malloc_init() {
	memset(&__malloc_info__, 0, sizeof(malloc_t));
	_malloc_buf = NULL;
	_malloc_buf_size = MALLOC_BUF_SIZE_DEF;
	_malloc_seg_size = MALLOC_SEG_SIZE_DEF;
}

void __malloc_buf_set(uint32_t buf_size, uint32_t seg_size) {
	_malloc_seg_size = ALIGN_UP(seg_size, 8);
	_malloc_buf_size = ALIGN_UP(buf_size, 8);

	if(_malloc_seg_size > 0 && _malloc_buf_size <= _malloc_seg_size)
		_malloc_buf_size = _malloc_seg_size * 16;
}

static void malloc_setup() {
	if(_malloc_buf != NULL ||
			_malloc_buf_size == 0 ||
			_malloc_seg_size == 0) {
		return;
	}

	_malloc_buf = proc_malloc_expand(_malloc_buf_size);
	_malloc_mem_tail = (uint32_t)_malloc_buf;
	__malloc_info__.expand = m_expand;
	__malloc_info__.shrink = m_shrink;
	__malloc_info__.get_mem_tail = m_get_mem_tail;
	__malloc_info__.seg_size = _malloc_seg_size;
	__malloc_info__.arg = NULL;
}

void __ewok_malloc_close() {
	if(_malloc_buf != NULL)
		proc_malloc_free();
}

size_t __size__(void* ptr) {
	return trunk_msize(&__malloc_info__, ptr); 
}

void *__malloc__(size_t size) {
	if(_malloc_buf == NULL) //malloc buf not setup
		malloc_setup();
	if(_malloc_buf == NULL)
		return NULL;
	return trunk_malloc(&__malloc_info__, size);
}

void __free__(void* p) {
	if(_malloc_buf == NULL || p == 0)
		return;
	trunk_free(&__malloc_info__, p);
}

void* malloc(size_t size) {
	proc_global_lock();
	void* ret = __malloc__(size);
	proc_global_unlock();
	return ret;
}

