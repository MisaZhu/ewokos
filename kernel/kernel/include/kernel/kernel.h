#ifndef KERNEL_H
#define KERNEL_H

#include <mm/mmu.h>
#include <kernel/boot.h>
#include <ewokos_config.h>

#define IPC_TIMEOUT_USEC 10000000 //ipc timeout as 10s
#define INTERRUPT_TIMEOUT_USEC 3000000 //interrupt timeout as 3s
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
	uint32_t kmalloc_size;
	uint32_t dma_size;
	uint32_t shm_contig_size;
} kernel_conf_t;

extern kernel_conf_t _kernel_config;
extern void load_kernel_config(void);

/*
 * Arch hooks that keep the common kernel/kernel/src/kernel.c free of
 * architecture #if macros. Weak defaults live in kernel.c; the platform
 * override (if any) lives in kernel/platform/<arch>/arch/common/src/kernel_arch.c.
 */
/* boot banner + memory-layout dump (address widths are arch specific) */
extern void arch_show_config(void);
/* very early per-arch entry fixup before bss clear (x86 masks interrupts) */
extern void arch_kernel_entry_early(void);
/* install the kernel view into a fresh per-process address space (page-table
   layout is arch specific) */
extern void arch_set_proc_vm(page_dir_entry_t* vm);
/* resolve the logical core a secondary/AP core binds to during bring-up */
extern uint32_t arch_slave_resolve_core(uint32_t boot_core_id, uint32_t detected_cid);

#endif
