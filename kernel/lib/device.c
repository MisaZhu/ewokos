#include <device.h>

int32_t dev_buffer_push(dev_buffer_t *buffer, char c, bool loop) { 
	if(buffer->buffer == NULL || buffer->buffer_size == 0)
		return -1;

	uint32_t at;
	if(buffer->size == buffer->buffer_size) {
		if(!loop)
			return -1;
		buffer->start++;
		if(buffer->start == buffer->buffer_size)
			buffer->start = 0;
		at = (buffer->start + buffer->size)-1;
	}
	else
		at = (buffer->start + buffer->size);

	if(at >= buffer->buffer_size) {
		at -= buffer->buffer_size;
	}
	((char*)buffer->buffer)[at] = c;
	if(buffer->size < buffer->buffer_size)
		buffer->size++;
	return 0;
}

int32_t dev_buffer_pop(dev_buffer_t *buffer, char* c) {
	if(buffer->size == 0)
		return -1;

	*c = ((char*)buffer->buffer)[buffer->start]; 
	buffer->start++;
	if(buffer->start == buffer->buffer_size)
		buffer->start = 0;
	buffer->size--;
	return 0;
}

