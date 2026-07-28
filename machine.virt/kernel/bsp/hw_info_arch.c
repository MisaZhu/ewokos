#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <stdbool.h>

#ifdef KERNEL_SMP
#include <kernel/core.h>
#endif



uint32_t _core_base_offset = 0;
uint32_t _pi4 = 0;
	
#define FB_SIZE (0*MB)

void sys_info_init_arch(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));

	_sys_info.phy_offset = 0x40000000;
	_sys_info.vector_base = 0x40000000;
	_sys_info.total_phy_mem_size = 1024*MB;
	_sys_info.total_usable_mem_size = _sys_info.total_phy_mem_size;
	//_core_base_offset =  0x01000000;
	_sys_info.mmio.phy_base = 0x8000000;
	strcpy(_sys_info.machine, "virt");

	_sys_info.total_usable_mem_size = _sys_info.total_phy_mem_size;
	if(_sys_info.total_usable_mem_size > (uint32_t)MAX_USABLE_MEM_SIZE)
		_sys_info.total_usable_mem_size = MAX_USABLE_MEM_SIZE;

#if __aarch64__
	strcpy(_sys_info.arch, "aarch64");
#elif __arm__
	strcpy(_sys_info.arch, "armv7");
#endif
	_sys_info.mmio.size = 128*MB;
	_sys_info.sys_dma.size = 32*MB;

	if(_sys_info.total_usable_mem_size <= 1*GB) {
		_sys_info.allocable_phy_mem_top = _sys_info.phy_offset +
			_sys_info.total_usable_mem_size - FB_SIZE;
	}
	else {
		_sys_info.allocable_phy_mem_top = _sys_info.phy_offset + _sys_info.total_usable_mem_size;
	}

#ifdef KERNEL_SMP
	_sys_info.cores = get_cpu_cores();
	timer_init();
#else
	_sys_info.cores = 1;
#endif

#if __aarch64__
	_sys_info.vector_base = (ewokos_addr_t)&interrupt_table_start;
#endif 
}

void arch_vm(page_dir_entry_t* vm) {
}


#ifdef KERNEL_SMP
#if __aarch64__
static unsigned long psci_cpu_on(unsigned long target_cpu, unsigned long entry_point)
{
    unsigned long ret;
    // Invoke PSCI_CPU_ON through HVC with the target CPU ID and entry address.
    __asm__ volatile(
        "mov x0, %1\n"          // PSCI_CPU_ON_64
        "mov x1, %2\n"          // target_cpu (MPIDR)
        "mov x2, %3\n"          // entry_point (physical address)
        "mov x3, 0\n"           // context_id (usually 0)
        "hvc #0\n"
        "mov %0, x0"            // Return value
        : "=r" (ret)
        : "r" (0xc4000003), "r" (target_cpu), "r" (entry_point)
        : "x0", "x1", "x2", "x3", "memory"
    );
    return ret; // 0 means success, non-zero indicates an error
}
extern char __entry[];
inline void __attribute__((optimize("O0"))) start_core(uint32_t core_id) {
    unsigned long ret = psci_cpu_on(core_id, __entry);
}
#else
static uint32_t psci_cpu_on(uint32_t target_cpu, uint32_t entry_point)
{
	uint32_t ret;
	__asm__ volatile(
		"mov r0, %1\n"
		"mov r1, %2\n"
		"mov r2, %3\n"
		"mov r3, %4\n"
		"hvc #0\n"
		"mov %0, r0\n"
		: "=r" (ret)
		: "r" (0x84000003u), "r" (target_cpu), "r" (entry_point), "r" (target_cpu)
		: "r0", "r1", "r2", "r3", "memory"
	);
	return ret;
}

extern char __entry[];
inline void __attribute__((optimize("O0"))) start_core(uint32_t core_id) {
	/*
	 * __entry is linked into the low physical image on ARM32 virt, so pass the
	 * symbol value directly as the PSCI reset vector. QEMU virt exposes MPIDR
	 * affinity values as linear cpu ids for the first cluster, so core_id can
	 * be passed through directly here.
	 */
	psci_cpu_on(core_id, (uint32_t)__entry);
}
#endif
#endif

void kalloc_arch(void) {
	kalloc_append(P2V(_sys_info.allocable_phy_mem_base), P2V(_sys_info.allocable_phy_mem_top));
}

int32_t  check_mem_map_arch(ewokos_addr_t phy_base, uint32_t size) {
	if(phy_base >= _sys_info.mmio.phy_base && size <= _sys_info.mmio.size)
		return 0;
	return -1;
}
