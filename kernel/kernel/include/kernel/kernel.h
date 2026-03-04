#ifndef KERNEL_H
#define KERNEL_H

#include <mm/mmu.h>
#include <kernel/boot.h>
#include <ewokos_config.h>

#define IPC_TIMEOUT_USEC 3000000 //ipc timeout as 3s
#define INTERRUPT_TIMEOUT_USEC 1000000 //ipc timeout as 1s
#define KERNEL_PROC_RUN_RECOUNT_SEC   2
#define SCHEDULE_FREQ_DEF     512 // usecs (timer/schedule)


typedef struct {
	uint32_t uptime_sec;
	uint64_t uptime_usec;
	vsyscall_info_t* vsyscall_info;
	page_dir_entry_t* kernel_vm;
} kernel_info_t;

extern kernel_info_t _kernel_info;

extern void set_vm(page_dir_entry_t* vm);

#define MAX_PROC_NUM_DEF  64
#define MAX_TASK_PER_PROC_DEF 64

typedef struct {
	uint32_t timer_freq;	
	uint32_t cores;
	uint32_t uart_baud;

	uint32_t max_proc_num;
	uint32_t max_task_num;
	uint32_t max_task_per_proc;
} kernel_conf_t;

extern kernel_conf_t _kernel_config;
extern void load_kernel_config(void);

#endif
