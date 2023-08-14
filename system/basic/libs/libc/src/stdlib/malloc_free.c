#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/trunkmem.h>

#define MALLOC_BUF_SIZE_DEF  (4*1024)
#define MALLOC_SEG_SIZE_DEF  128

static malloc_t __malloc_info__;
static char* _malloc_buf;
static uint32_t _malloc_mem_tail;
static uint32_t _malloc_buf_size;
static uint32_t _malloc_seg_size;

static void *malloc_raw(size_t size) {
	return (void*)syscall1(SYS_MALLOC, (int32_t)size);
}

static void free_raw(void* ptr) {
	syscall1(SYS_FREE, (int32_t)ptr);
}

static void m_shrink(void* arg, int32_t pages) {
	(void)arg;
	_malloc_mem_tail -= pages * __malloc_info__.seg_size;
}

static int32_t m_expand(void* arg, int32_t pages) {
	(void)arg;
	uint32_t to = _malloc_mem_tail + (pages * __malloc_info__.seg_size);
	if(to > ((uint32_t)(_malloc_buf) + _malloc_buf_size)) //over flow
		return -1;

	_malloc_mem_tail = to;
	return 0;
}


static void* m_get_mem_tail(void* arg) {
	(void)arg;
	return (void*)_malloc_mem_tail;
}

void __malloc_init() {
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
			_malloc_seg_size == 0)
		return;

	_malloc_buf = malloc_raw(_malloc_buf_size);
	_malloc_mem_tail = (uint32_t)_malloc_buf;
	__malloc_info__.expand = m_expand;
	__malloc_info__.shrink = m_shrink;
	__malloc_info__.get_mem_tail = m_get_mem_tail;
	__malloc_info__.seg_size = _malloc_seg_size;
	__malloc_info__.arg = NULL;
}

void __malloc_close() {
	if(_malloc_buf != NULL)
		free_raw(_malloc_buf);
}

static void *m_malloc(uint32_t size) {
	if(_malloc_buf == NULL) //malloc buf not setup
		malloc_setup();
	if(_malloc_buf == NULL)
		return NULL;
	return trunk_malloc(&__malloc_info__, size);
}

static void m_free(void* p) {
	if(_malloc_buf == NULL || p == 0)
		return;
	trunk_free(&__malloc_info__, p);
}

void *malloc(size_t size) {
	void* ret = NULL;
	ret = m_malloc(size);
	if(ret == NULL) {
		ret = malloc_raw(size);
	}
	return ret;
}

void free(void* ptr) {
	uint32_t p = (uint32_t)ptr;
	if(p == 0)
		return;

	if(p > (uint32_t)_malloc_buf && p < _malloc_mem_tail)
		m_free(p);
	else  {
		free_raw(ptr);
	}	
}

void* realloc(void* s, uint32_t new_size) {
	if(new_size == 0) {
		if(s != NULL)
			free(s);
		return NULL;
	}

	if(s == NULL)
		return malloc(new_size);
	
	uint32_t old_size = 0;
	uint32_t p = (uint32_t)s;
	if(p > (uint32_t)_malloc_buf && p < _malloc_mem_tail)
		old_size = trunk_msize(&__malloc_info__, s); 
	else
		old_size = syscall1(SYS_MSIZE, (int32_t)s); 
	
	if(old_size >= new_size)
		return s;

	void* n = malloc(new_size);
	if(n != NULL)
		memcpy(n, s, old_size);
	free(s);
	return n;
}