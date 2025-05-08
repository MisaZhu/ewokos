#ifndef KERNEL_H
#define KERNEL_H

#include <mm/mmu.h>
#include <ewokos_config.h>

#define IPC_TIMEOUT_USEC 100000 //ipc timeout as 100ms
#define KERNEL_PROC_RUN_RECOUNT_SEC   2
#define SCHEDULE_FREQ_DEF     512 // usecs (timer/schedule)

extern uint32_t _kernel_sec;
extern uint64_t _kernel_usec;
extern vsyscall_info_t* _kernel_vsyscall_info;

extern char _kernel_start[];
extern char _kernel_end[];
extern char _kernel_sp[];

extern char _bss_start[];
extern char _bss_end[];

extern page_dir_entry_t* _kernel_vm;
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
