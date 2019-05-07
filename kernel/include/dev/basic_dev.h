#ifndef BASIC_DEVICE_H
#define BASIC_DEVICE_H

#include <types.h>
#include <device.h>
#include <sconf.h>

bool dev_init(void);
bool conf_dev_init(sconf_t* conf);

int32_t dev_info(int32_t type_id, void* info);

int32_t dev_char_read(int32_t type_id, void* buf, uint32_t size);
int32_t dev_char_write(int32_t type_id, void* buf, uint32_t size);

int32_t dev_block_read(int32_t type_id, uint32_t block);
int32_t dev_block_read_done(int32_t type_id, void* buf);
int32_t dev_block_write(int32_t type_id, uint32_t block, void* buf);
int32_t dev_block_write_done(int32_t type_id);

/*some devices     */
extern bool uart_init(void);
extern void uart_handle(void);
extern int32_t dev_uart_read(int16_t id, void* buf, uint32_t size);
extern int32_t dev_uart_write(int16_t id, void* buf, uint32_t size);

extern bool keyboard_init(void);
extern void keyboard_handle(void);
extern int32_t dev_keyboard_read(int16_t id, void* buf, uint32_t size);

extern bool mouse_init(void);
extern void mouse_handle(void);
extern int32_t dev_mouse_read(int16_t id, void* buf, uint32_t size);

extern bool fb_init(sconf_t* conf);
extern int32_t dev_fb_info(int16_t id, void* info);
extern int32_t dev_fb_write(int16_t id, void* buf, uint32_t size);

extern bool sdc_init(void);
extern void sdc_handle(void);
extern int32_t dev_sdc_read(int16_t id, uint32_t block);
extern int32_t dev_sdc_read_done(int16_t id, void* buf);
extern int32_t dev_sdc_write(int16_t id, uint32_t block, void* buf);
extern int32_t dev_sdc_write_done(int16_t id);
/********************/

#endif
