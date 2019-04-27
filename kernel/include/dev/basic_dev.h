#ifndef BASIC_DEVICE_H
#define BASIC_DEVICE_H

#include <types.h>
#include <device.h>

bool dev_init();

int32_t dev_info(int32_t type_id, void* info);

int32_t dev_char_read(int32_t type_id, void* buf, uint32_t size);
int32_t dev_char_write(int32_t type_id, void* buf, uint32_t size);

int32_t dev_block_read(int32_t type_id, uint32_t block);
int32_t dev_block_read_done(int32_t type_id, void* buf);
int32_t dev_block_write(int32_t type_id, uint32_t block, void* buf);
int32_t dev_block_write_done(int32_t type_id);


#endif
