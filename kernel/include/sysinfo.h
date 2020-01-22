#ifndef SYSINFO_H
#define SYSINFO_H

typedef struct {
	char machine[32];
	uint32_t free_mem;
	uint32_t shm_mem;
	uint32_t total_mem;
	uint32_t kernel_tic;
} sysinfo_t;

#endif

