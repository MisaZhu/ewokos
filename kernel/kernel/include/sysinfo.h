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
	char           machine[MACHINE_MAX];
	char           arch[ARCH_MAX];
	ewokos_addr_t  total_phy_mem_size;
	uint32_t       kmalloc_size;
	ewokos_addr_t  total_usable_mem_size;
	ewokos_addr_t  phy_offset;
	ewokos_addr_t  kernel_base;
	ewokos_addr_t  vector_base;

	ewokos_addr_t  allocable_phy_mem_top;
	ewokos_addr_t  allocable_phy_mem_base;

	mmio_info_t    mmio;
	dma_info_t     sys_dma;
	/* reserved physically-contiguous slab backing IPC_CONTIG segments.
	   size == 0 means the slab is not configured (contig shm fails) */
	dma_info_t     shm_contig;
	uint32_t       cores;
	uint32_t       core_idles[MAX_CORE_NUM];
	uint32_t       core_procs[MAX_CORE_NUM];
	uint32_t       core_kernels[MAX_CORE_NUM];

	uint32_t       max_proc_num;
	uint32_t       max_task_num;
	uint32_t       max_task_per_proc;
	uint32_t       page_size;
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

/*
 * Sub-tick clock interpolation base, republished on every scheduler tick.
 *
 * kernel_usec alone is quantized to the tick period (~976us at the default
 * timer_freq=1024), which is orders of magnitude too coarse for nanosleep()
 * and CLOCK_MONOTONIC. Platforms whose free-running counter is readable from
 * userspace (CNTVCT_EL0 on arm/aarch64, `time` on riscv) also publish the
 * counter value captured at the same instant as kernel_usec, so libc can
 * interpolate between ticks with no syscall at all:
 *
 *   nsec = base_usec*1000 + (cnt - base_cnt) * nsec_per_cnt
 *
 * nsec_per_cnt is precomputed here as a 32.32 fixed-point value so the reader
 * needs only a 64x64->hi64 multiply and never a 64-bit divide (arm32 has no
 * hardware divide and __udivdi3 in a spin loop would dominate the wait).
 *
 * seq guards the (base_cnt, base_usec, kernel_usec) triple: the kernel makes it
 * odd while updating and even when done, and a reader that sees it change
 * retries. Without that a torn read is off by a whole tick - the exact error
 * this struct exists to remove.
 *
 * fine_cnt_hz == 0 means "no user-readable counter here"; every other field is
 * then meaningless and the reader must use kernel_usec alone.
 *
 * tick_usec is the measured tick period, which lets libc reserve exactly one
 * tick of spin slack when it needs a precise long sleep (see proc.c). It is
 * measured rather than derived from timer_freq because some BSPs do not honour
 * that setting (machines/virt.riscv hardcodes a 40000-count reload).
 */
typedef struct {
	uint32_t seq;
	uint32_t fine_cnt_hz;
	uint32_t nsec_q32_hi;
	uint32_t nsec_q32_lo;
	uint32_t tick_usec;
	uint32_t _reserved;
	uint64_t base_cnt;
} vsyscall_fine_clock_t;

typedef struct {
	uint64_t kernel_usec;
	proc_base_info_t proc_info[MAX_PROC_NUM];
	/*
	 * Appended last on purpose. proc_info[] is read by libc's vsyscall
	 * helpers (proc_getpid/proc_ppid, IPC peer lookup) and those objects
	 * are built separately from the kernel, so any field inserted before
	 * the array shifts it and silently desyncs a partially-rebuilt tree.
	 * Appending keeps the change ABI-compatible: sizeof() grows but the
	 * whole struct still rounds to the same KERNEL_VSYSCALL_INFO_SIZE page
	 * allocation, and every pre-existing offset is untouched.
	 */
	vsyscall_fine_clock_t fine;
} vsyscall_info_t;

#endif
