#include <device.h>

int32_t dev_buffer_push(dev_buffer_t *buffer, char c, bool replace) { 
	if(buffer->buffer == NULL || buffer->buffer_size == 0)
		return -1;

	if(buffer->size >= buffer->buffer_size && !replace)
		return -1;

	uint32_t at = (buffer->start + buffer->size);
	if(at >= buffer->buffer_size) {
		at -= buffer->buffer_size;
		buffer->start = at+1;
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
	buffer->size--;
	buffer->start++;
	if(buffer->start >= buffer->buffer_size)
		buffer->start = 0;
	return 0;
}

int32_t dev_buffer_push_int(dev_buffer_t *buffer, int32_t i, bool replace) { 
	if(buffer->buffer == NULL || buffer->buffer_size == 0)
		return -1;

	if(buffer->size >= buffer->buffer_size && !replace)
		return -1;

	uint32_t at = (buffer->start + buffer->size);
	if(at >= buffer->buffer_size) {
		at -= buffer->buffer_size;
		buffer->start = at+1;
	}
	((int32_t*)buffer->buffer)[at] = i;

	if(buffer->size < buffer->buffer_size)
		buffer->size++;
	return 0;
}

int32_t dev_buffer_pop_int(dev_buffer_t *buffer, int32_t* i) {
	if(buffer->size == 0)
		return -1;

	*i = ((int32_t*)buffer->buffer)[buffer->start]; 
	buffer->size--;
	buffer->start++;
	if(buffer->start >= buffer->buffer_size)
		buffer->start = 0;
	return 0;
}

