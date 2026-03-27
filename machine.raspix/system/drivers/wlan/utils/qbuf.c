#include <stdlib.h>
#include <types.h>
#include <string.h>

#include "qbuf.h"
#include "log.h"




queue_buffer_t *queue_buffer_alloc(int qsize, int bsize){
	int i;
	queue_buffer_t *qbuf = malloc(sizeof(queue_buffer_t));
	if(qbuf){
		qbuf->bsize = bsize;
		qbuf->push_idx = 0;
		qbuf->pop_idx = 0;
		qbuf->bufs = malloc(sizeof(buf_t) * qsize);
		if(!qbuf->bufs){
			free(qbuf);
			return NULL;
		}
		for(i = 0; i < qsize; i++){
			qbuf->bufs[i].size = 0;
			qbuf->bufs[i].data = malloc(bsize);
			if(qbuf->bufs[i].data == NULL)
				break;
		}
		qbuf->qsize = i;
		klog("malloc queue buffer %d size %d\n", i,  bsize);
	}
	return qbuf;
}

int queue_buffer_push(queue_buffer_t *qbuf, uint8_t* buf, int size){
	buf_t *dst = NULL;
	if(qbuf->push_idx - qbuf->pop_idx < qbuf->qsize){
		int idx = qbuf->push_idx % qbuf->qsize;
		dst = &qbuf->bufs[idx];
		qbuf->push_idx++;
	}
	
	if(dst){
		size = min(qbuf->bsize, size);
		memcpy(dst->data, buf, size);
		dst->size = size;
		return size;
	}
	return 0;
}

int queue_buffer_pop(queue_buffer_t *qbuf, uint8_t* buf, int size){
	buf_t *src = NULL;
	if(qbuf->push_idx > qbuf->pop_idx){
		int idx = qbuf->pop_idx % qbuf->qsize;
 		src = &qbuf->bufs[idx];
		qbuf->pop_idx++;
	}
	if(qbuf->push_idx >= qbuf->qsize && qbuf->pop_idx >= qbuf->qsize){
		qbuf->push_idx -= qbuf->qsize;
		qbuf->pop_idx -= qbuf->qsize;
	}

	if(src){
		size = min(src->size, size);
		memcpy(buf, src->data, size);
		return size;
	}
	return 0;
}

int queue_buffer_check(queue_buffer_t *qbuf){
	return (qbuf->push_idx - qbuf->pop_idx);
}
