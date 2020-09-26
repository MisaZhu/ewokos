#ifndef SYSINFO_H
#define SYSINFO_H

typedef struct {
	uint32_t phy_base;
	uint32_t v_base;
	uint32_t size;
} mmio_info_t;

typedef struct {
	char machine[32];
	uint32_t kfs;
	mmio_info_t mmio_info;
	uint32_t free_mem;
	uint32_t shm_mem;
	uint32_t total_mem;
	uint32_t kernel_sec;
} sysinfo_t;

#endif

