#include <dev/basic_dev.h>
#include <proc.h>

int32_t dev_buffer_push(dev_buffer_t *buffer, char c, bool replace) { 
	if(buffer->buffer == NULL || buffer->buffer_size == 0)
		return -1;

	if(buffer->size >= buffer->buffer_size && !replace)
		return -1;

	uint32_t at = (buffer->offset + buffer->size);
	if(at >= buffer->buffer_size)
		at -= buffer->buffer_size;
	buffer->buffer[at] = c;

	if(buffer->size < buffer->buffer_size)
		buffer->size++;
	return 0;
}

int32_t dev_buffer_pop(dev_buffer_t *buffer, char* c) {
	if(buffer->size == 0)
		return -1;

	*c = buffer->buffer[buffer->offset]; 
	buffer->size--;
	buffer->offset++;
	if(buffer->offset >= buffer->buffer_size)
		buffer->offset = 0;
	return 0;
}


/*some devices     */
int32_t dev_fb_info(int16_t id, void* info);
/*******************/

int32_t dev_info(int32_t type_id, void* info) {
	int16_t type, id;
	type = (type_id & 0xffff0000) >> 16;
	id = (type_id & 0xffff);
	(void)id;

	switch(type) {
	case DEV_FRAME_BUFFER:
		return dev_fb_info(id, info);
	}
	return -1;
}

/*some char devices*/
int32_t dev_keyboard_read(int16_t id, void* buf, uint32_t size);

int32_t dev_uart_read(int16_t id, void* buf, uint32_t size);
int32_t dev_uart_write(int16_t id, void* buf, uint32_t size);

int32_t dev_fb_write(int16_t id, void* buf, uint32_t size);
/*******************/

int32_t dev_char_read(int32_t type_id, void* buf, uint32_t size) {
	if(_current_proc->owner > 0)
		return -1;

	int16_t type, id;
	type = (type_id & 0xffff0000) >> 16;
	id = (type_id & 0xffff);

	switch(type) {
	case DEV_KEYBOARD:
		return dev_keyboard_read(id, buf, size);
	case DEV_UART:
		return dev_uart_read(id, buf, size);
	}

	return -1;
}

int32_t dev_char_write(int32_t type_id, void* buf, uint32_t size) {
	if(_current_proc->owner > 0)
		return -1;

	int16_t type, id;
	type = (type_id & 0xffff0000) >> 16;
	id = (type_id & 0xffff);
	(void)id;

	switch(type) {
	case DEV_UART:
		return dev_uart_write(id, buf, size);
	case DEV_FRAME_BUFFER:
		return dev_fb_write(id, buf, size);
	}
	return -1;
}

/*some block devices*/
int32_t dev_sdc_read(int16_t id, uint32_t block);
int32_t dev_sdc_read_done(int16_t id, void* buf);
int32_t dev_sdc_write(int16_t id, uint32_t block, void* buf);
int32_t dev_sdc_write_done(int16_t id);
/********************/

int32_t dev_block_read(int32_t type_id, uint32_t block) {
	if(_current_proc->owner > 0)
		return -1;

	int16_t type, id;
	type = (type_id & 0xffff0000) >> 16;
	id = (type_id & 0xffff);

	switch(type) {
	case DEV_SDC:
		return dev_sdc_read(id, block);
	}

	return -1;
}

int32_t dev_block_read_done(int32_t type_id, void* buf) {
	if(_current_proc->owner > 0)
		return -1;

	int16_t type, id;
	type = (type_id & 0xffff0000) >> 16;
	id = (type_id & 0xffff);

	switch(type) {
	case DEV_SDC:
		return dev_sdc_read_done(id, buf);
	}
	return -1;
}

int32_t dev_block_write(int32_t type_id, uint32_t block, void* buf) {
	if(_current_proc->owner > 0)
		return -1;

	int16_t type, id;
	type = (type_id & 0xffff0000) >> 16;
	id = (type_id & 0xffff);
	(void)id;

	switch(type) {
	case DEV_SDC:
		return dev_sdc_write(id, block, buf);
	}
	return -1;
}

int32_t dev_block_write_done(int32_t type_id) {
	if(_current_proc->owner > 0)
		return -1;

	int16_t type, id;
	type = (type_id & 0xffff0000) >> 16;
	id = (type_id & 0xffff);
	(void)id;

	switch(type) {
	case DEV_SDC:
		return dev_sdc_write_done(id);
	}
	return -1;
}

