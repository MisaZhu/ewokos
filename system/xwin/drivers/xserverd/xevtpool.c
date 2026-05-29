#include "xevtpool.h"
#include <stdlib.h>
#include <string.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfs.h>

#define XEVT_MAX 128

typedef struct xevt_pool_item {
	xevent_t evt;	
	struct xevt_pool_item* next;
} xevt_item_t;

typedef struct evt_pool {
	xevt_item_t* head;
	xevt_item_t* tail;
	int pid;
	uint32_t node;
	uint32_t num;
	struct evt_pool *next;
	struct evt_pool *prev;
} xevt_pool_t;

static xevt_pool_t* _xevt_pool_head = NULL;

static uint32_t create_evt_node(void) {
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	info.type = FS_TYPE_CHAR | FS_TYPE_ANNOUNIMOUS;
	info.stat.uid = getuid();
	info.stat.gid = getgid();
	info.stat.mode = 0600;
	if(vfs_new_node(&info, 0, true, false) != 0)
		return 0;
	return info.node;
}

static inline xevt_pool_t* get_pool(int pid) {
	xevt_pool_t* p = _xevt_pool_head;
	while(p != NULL) {
		if(proc_getpid(p->pid) == proc_getpid(pid))
			return p;
		p = p->next;
	}
	return NULL;
}

static xevt_pool_t* get_or_new_pool(int pid) {
	xevt_pool_t* pool = get_pool(pid);
	if(pool != NULL)
		return pool;

	pool = (xevt_pool_t*)calloc(1, sizeof(xevt_pool_t));
	if(pool == NULL)
		return NULL;
	pool->pid = proc_getpid(pid);
	pool->node = create_evt_node();
	if(pool->node == 0) {
		free(pool);
		return NULL;
	}
	if(_xevt_pool_head != NULL) {
		_xevt_pool_head->prev = pool;
		pool->next = _xevt_pool_head;
	}
	_xevt_pool_head = pool;
	return pool;
}

static void xevent_pool_push(xevt_pool_t* pool, xevent_t* evt) {
	xevt_item_t* item = (xevt_item_t*)calloc(1, sizeof(xevt_item_t));
	memcpy(&item->evt, evt, sizeof(xevent_t));

	if(pool->head == NULL)
		pool->head = item;

	if(pool->tail != NULL)
		pool->tail->next = item;
	pool->tail = item;
	pool->num++;
}

static inline bool xevent_pool_pop(xevt_pool_t* pool, xevent_t* evt) {
	if(pool->head == NULL || pool->num == 0)
		return false;

	if(evt != NULL)
		memcpy(evt, &pool->head->evt, sizeof(xevent_t));

	xevt_item_t* item = pool->head;
	pool->head = item->next;
	if(pool->head == NULL)
		pool->tail = NULL;
	free(item);
	pool->num--;
	return true;
}

void xevent_push(int pid, xevent_t* evt) {
	xevt_pool_t* pool = get_or_new_pool(pid);
	if(pool == NULL)
		return;
	if(pool->num >= XEVT_MAX)
		xevent_pool_pop(pool, NULL);
	xevent_pool_push(pool, evt);
}

bool xevent_pop(int pid, xevent_t* evt) {
	xevt_pool_t* pool = get_pool(pid);
	if(pool == NULL)
		return false;
	return xevent_pool_pop(pool, evt);
}

uint32_t xevent_get_node(int pid) {
	xevt_pool_t* pool = get_or_new_pool(pid);
	if(pool == NULL)
		return 0;
	return pool->node;
}

void xevent_remove(int pid) {
	xevt_pool_t* pool = get_pool(pid);
	if(pool == NULL)
		return;
	
	xevt_item_t* item = pool->head;
	while(item != NULL) {
		xevt_item_t* it = item;
		item = item->next;
		free(it);
	}

	if(pool == _xevt_pool_head)
		_xevt_pool_head = pool->next;
	if(pool->next != NULL)
		pool->next->prev = pool->prev;
	if(pool->prev != NULL)
		pool->prev->next = pool->next;
	if(pool->node != 0)
		vfs_del_node(pool->node);
	free(pool);
}

void xevent_pool_init(void) {
	_xevt_pool_head = NULL;
}
