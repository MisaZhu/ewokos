#ifndef KFILE_H
#define KFILE_H

#include <types.h>

#define KF_READ 	0x0
#define KF_WRITE 0x1

typedef struct k_file {
	uint32_t node_addr;
	uint16_t ref_r;
	uint16_t ref_w;
} k_file_t;

int32_t kf_open(uint32_t node_addr, int32_t flags);
void kf_close(int32_t fd);
void kf_unref(k_file_t* kf, uint32_t flags);
void kf_ref(k_file_t* kf, uint32_t flags);
uint32_t kf_node_addr(int32_t pid, int32_t fd);

#endif
