#ifndef SYSINFO_H
#define SYSINFO_H

typedef struct {
	uint32_t phy_base;
	uint32_t v_base;
	uint32_t size;
} mmio_info_t;

typedef struct {
	uint32_t phy_base;
	uint32_t v_base;
	uint32_t size;
} fb_info_t;

typedef struct {
	uint32_t free;
	uint32_t shared;
} mem_info_t;

typedef struct {
/*static attr*/
	char machine[32];
	uint32_t kfs;
	uint32_t phy_mem_size;

	mmio_info_t mmio;
	fb_info_t fb;

/*dynamic attr*/
	mem_info_t mem;
	uint32_t kernel_sec;
} sys_info_t;

#endif

