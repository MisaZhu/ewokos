#include <dev/sdc.h>
//TODO

int32_t sdc_init() {
	return 0;
}

int32_t sdc_read_block(int32_t block) {
	(void)block;
	return 0;
}

inline int32_t sdc_read_done(char* buf) {
	(void)buf;
	return 0;
}

int32_t sdc_write_block(int32_t block, const char* buf) {
	(void)block;
	(void)buf;
	return 0;
}

inline int32_t sdc_write_done() {
	return 0;
}

void sdc_handle() {
}

