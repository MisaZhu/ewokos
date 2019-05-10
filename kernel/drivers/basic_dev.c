#include <dev/basic_dev.h>
#include <proc.h>

/*some devices     */
extern bool uart_init(void);
extern bool keyboard_init(void);
extern bool mouse_init(void);
extern bool sdc_init(void);

bool dev_init() {
	if(!uart_init())
		return false;
	if(!keyboard_init())
		return false;
	if(!mouse_init())
		return false;
	if(!sdc_init())
		return false;
	return true;
}

extern bool fb_init(sconf_t* conf);

bool conf_dev_init(sconf_t* conf) {
	if(!fb_init(conf))
		return false;
	return true;
}

/*some devices     */
extern int32_t dev_fb_info(int16_t id, void* info);
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
extern int32_t dev_keyboard_read(int16_t id, void* buf, uint32_t size);
extern int32_t dev_mouse_read(int16_t id, void* buf, uint32_t size);

extern int32_t dev_uart_read(int16_t id, void* buf, uint32_t size);
extern int32_t dev_uart_write(int16_t id, void* buf, uint32_t size);

extern int32_t dev_fb_write(int16_t id, void* buf, uint32_t size);
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
	case DEV_MOUSE:
		return dev_mouse_read(id, buf, size);
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
extern int32_t dev_sdc_read(int16_t id, uint32_t block);
extern int32_t dev_sdc_read_done(int16_t id, void* buf);
extern int32_t dev_sdc_write(int16_t id, uint32_t block, void* buf);
extern int32_t dev_sdc_write_done(int16_t id);
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
