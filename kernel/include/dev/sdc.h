#ifndef SDC_H
#define SDC_H

#include <types.h>

#define SDC_BLOCK_SIZE 1024

int32_t sdc_init();
int32_t sdc_read_block(int32_t block);
int32_t sdc_read_done(char* buf);
int32_t sdc_write_block(int32_t block, const char* buf);
int32_t sdc_write_done();
void sdc_handle();

#endif
