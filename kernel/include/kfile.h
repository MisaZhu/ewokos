#ifndef KFILE_H
#define KFILE_H

#include <types.h>

typedef struct kfile {
	uint32_t node_addr;
	uint16_t ref_r;
	uint16_t ref_w;
} kfile_t;

void kf_init();
int32_t kf_open(uint32_t node_addr, int32_t wr);
void kf_close(int32_t fd);
void kf_unref(kfile_t* kf, uint32_t wr);
void kf_ref(kfile_t* kf, uint32_t wr);
int32_t kf_get_ref(uint32_t node_addr, uint32_t wr);
uint32_t kf_node_addr(int32_t pid, int32_t fd);

#endif
