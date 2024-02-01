#include <ewoksys/charbuf.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUF_DEF_SIZE 128


charbuf_t* charbuf_new(uint32_t buf_size) {
	if(buf_size == 0)
		buf_size = BUF_DEF_SIZE;

	charbuf_t* buffer = (charbuf_t*)malloc(sizeof(charbuf_t));
	if(buffer == NULL)
		return NULL;
	memset(buffer, 0, sizeof(charbuf_t));	

	buffer->buffer = (char*)malloc(buf_size);
	if(buffer->buffer == NULL) {
		free(buffer);
		return NULL;
	}
	memset(buffer->buffer, 0, buf_size);

	buffer->buf_size = buf_size;
	return buffer;
}

int32_t charbuf_push(charbuf_t *buffer, char c, bool loop) { 
	if(buffer->buffer == NULL)
		return -1;

	uint32_t at;
	if(buffer->size == buffer->buf_size) {
		if(!loop)
			return -1;
		buffer->start++;
		if(buffer->start == buffer->buf_size)
			buffer->start = 0;
		at = (buffer->start + buffer->size)-1;
	}
	else
		at = (buffer->start + buffer->size);

	if(at >= buffer->buf_size) {
		at -= buffer->buf_size;
	}
	((char*)buffer->buffer)[at] = c;
	if(buffer->size < buffer->buf_size)
		buffer->size++;
	return 0;
}

int32_t charbuf_pop(charbuf_t *buffer, char* c) {
	if(buffer->size == 0)
		return -1;

	*c = ((char*)buffer->buffer)[buffer->start]; 
	buffer->start++;
	if(buffer->start == buffer->buf_size)
		buffer->start = 0;
	buffer->size--;
	return 0;
}

void charbuf_free(charbuf_t* buffer) {
	if(buffer == NULL)
		return;

	if(buffer->buffer != NULL)
		free(buffer->buffer);
	free(buffer);
}

#ifdef __cplusplus
}
#endif

