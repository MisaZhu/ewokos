#ifndef __QUEUE_T__
#define __QUEUE_T__ 

#include <types.h>

typedef struct _buf{
	int size;
	uint8_t *data;
}buf_t;

typedef struct _queue_buffer{
    int qsize;
    int bsize;
    buf_t *bufs;
    int push_idx;
    int pop_idx;
}queue_buffer_t;

queue_buffer_t *queue_buffer_alloc(int qsize, int bsize);
int queue_buffer_push(queue_buffer_t *qbuf, uint8_t* buf, int size);
int queue_buffer_pop(queue_buffer_t *qbuf, uint8_t* buf, int size);
int queue_buffer_check(queue_buffer_t *qbuf);
#endif