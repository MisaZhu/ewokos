#include <buffer.h>
#include <kstring.h>

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
	if(buffer->size == 0) {//ready for write.
		size = size < BUFFER_SIZE ? size : BUFFER_SIZE;
		memcpy(buffer->buffer, buf, size);
		buffer->size = size;
		buffer->offset = 0;
		return size;
	}
	return 0;
}

