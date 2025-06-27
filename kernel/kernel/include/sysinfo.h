#ifndef SYSINFO_H
#define SYSINFO_H

#include <syscalls.h>
#include <stdint.h>
#include <ewokos_config.h>

typedef struct {
	ewokos_addr_t phy_base;
	ewokos_addr_t v_base;
	uint32_t size;
} mmio_info_t;

typedef struct {
	ewokos_addr_t phy_base;
	ewokos_addr_t v_base;
	uint32_t size;
	uint32_t max_size;
} gpu_info_t;

typedef struct {
	ewokos_addr_t free;
	ewokos_addr_t kfree;
	ewokos_addr_t shared;
} mem_info_t;

typedef struct {
	uint32_t size;
	ewokos_addr_t phy_base;
	ewokos_addr_t v_base;
} dma_info_t;

#define MAX_CORE_NUM 8
#define MAX_PROC_NUM 512
#define MACHINE_MAX  32
#define ARCH_MAX     16

/*static attr*/
typedef struct {
	char        machine[MACHINE_MAX];
	char        arch[ARCH_MAX];
	ewokos_addr_t    total_phy_mem_size;
	uint32_t         kmalloc_size;
	ewokos_addr_t    total_usable_mem_size;
	ewokos_addr_t    phy_offset;
	ewokos_addr_t    kernel_base;
	ewokos_addr_t    vector_base;

	mmio_info_t mmio;
	dma_info_t  sys_dma;
	gpu_info_t  gpu;
	uint32_t    cores;
	uint32_t    core_idles[MAX_CORE_NUM];

	uint32_t    max_proc_num;
	uint32_t    max_task_num;
	uint32_t    max_task_per_proc;
} sys_info_t;

/*dynamic attr*/
typedef struct {
	mem_info_t mem;
	uint64_t kernel_usec;
	uint32_t svc_total;
	uint32_t svc_counter[SYS_CALL_NUM];
} sys_state_t;

typedef struct {
	uint32_t uuid;
	int32_t  father_pid;
	int32_t  type;
} proc_base_info_t;

typedef struct {
	uint64_t kernel_usec;
	proc_base_info_t proc_info[MAX_PROC_NUM];
} vsyscall_info_t;

#endif

