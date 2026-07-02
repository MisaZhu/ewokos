#include <ewoksys/buffer.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


int32_t buffer_is_empty(buffer_t* buffer) {
	int32_t rest = buffer->size - buffer->offset;
	if(rest > 0)
		return 0;
	return 1;
}

int32_t buffer_read(buffer_t* buffer, void* buf, int32_t size) {
	int32_t rest = buffer->size - buffer->offset;
	if(rest > 0 && rest <= BUFFER_SIZE) {//ready for read.
		size = size < rest ? size : rest;
		memcpy(buf, buffer->buffer+buffer->offset, size);
		buffer->offset += size;
		if(buffer->offset == buffer->size) {
			buffer->offset = 0;
			buffer->size = 0;
		}
		return size;
	}
	return 0;
}

int32_t buffer_write(buffer_t* buffer, const void* buf, int32_t size) {
	int32_t rest = buffer->size - buffer->offset;
	if(rest < 0 || rest > BUFFER_SIZE)
		return 0;

	if(rest == 0) {
		buffer->size = 0;
		buffer->offset = 0;
	}
	else if(buffer->offset > 0) {
		memmove(buffer->buffer, buffer->buffer + buffer->offset, rest);
		buffer->size = rest;
		buffer->offset = 0;
	}

	int32_t free_sz = BUFFER_SIZE - buffer->size;
	if(free_sz <= 0)
		return 0;

	size = size < free_sz ? size : free_sz;
	memcpy(buffer->buffer + buffer->size, buf, size);
	buffer->size += size;
	return size;
	return 0;
}

#ifdef __cplusplus
}
#endif
