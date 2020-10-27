#include <sys/charbuf.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


void charbuf_init(charbuf_t *buffer) {
	memset(buffer, 0, sizeof(charbuf_t));	
}

int32_t charbuf_push(charbuf_t *buffer, char c, bool loop) { 
	if(buffer->buffer == NULL)
		return -1;

	uint32_t at;
	if(buffer->size == CHAR_BUF_MAX) {
		if(!loop)
			return -1;
		buffer->start++;
		if(buffer->start == CHAR_BUF_MAX)
			buffer->start = 0;
		at = (buffer->start + buffer->size)-1;
	}
	else
		at = (buffer->start + buffer->size);

	if(at >= CHAR_BUF_MAX) {
		at -= CHAR_BUF_MAX;
	}
	((char*)buffer->buffer)[at] = c;
	if(buffer->size < CHAR_BUF_MAX)
		buffer->size++;
	return 0;
}

int32_t charbuf_pop(charbuf_t *buffer, char* c) {
	if(buffer->size == 0)
		return -1;

	*c = ((char*)buffer->buffer)[buffer->start]; 
	buffer->start++;
	if(buffer->start == CHAR_BUF_MAX)
		buffer->start = 0;
	buffer->size--;
	return 0;
}

#ifdef __cplusplus
}
#endif

