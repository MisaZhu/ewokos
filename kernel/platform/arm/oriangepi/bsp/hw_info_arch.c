#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <stdbool.h>

#ifdef KERNEL_SMP
#include <kernel/core.h>
#endif


uint32_t _allocable_phy_mem_top = 0;
uint32_t _allocable_phy_mem_base = 0;
uint32_t _pi4 = 0;
	
#define FB_SIZE 64*MB

void sys_info_init_arch(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));
	strcpy(_sys_info.machine, "orange-pi-2w");
	_sys_info.phy_offset = 0x40000000;
    _sys_info.vector_base = 0x40000000;
	_sys_info.total_usable_mem_size = 128*MB;
	_sys_info.mmio.phy_base = 0x00000000;

	if(_sys_info.total_usable_mem_size > (uint32_t)MAX_USABLE_MEM_SIZE)
		_sys_info.total_usable_mem_size = MAX_USABLE_MEM_SIZE;

	strcpy(_sys_info.arch, "armv7");
	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 128*MB;

	_allocable_phy_mem_base = V2P(get_allocable_start());
	_sys_info.dma.phy_base = _allocable_phy_mem_base;
	_sys_info.dma.size = DMA_SIZE;
	_sys_info.dma.v_base = DMA_BASE;
	_allocable_phy_mem_base += DMA_SIZE;

	if(_sys_info.total_usable_mem_size <= 1*GB) {
		_allocable_phy_mem_top = _sys_info.phy_offset +
				_sys_info.total_usable_mem_size - FB_SIZE;
	}
	else {
		_allocable_phy_mem_top = _sys_info.phy_offset + _sys_info.total_usable_mem_size;
	}
#ifdef KERNEL_SMP
	_sys_info.cores = get_cpu_cores();
#else
	_sys_info.cores = 1;
#endif
}

void arch_vm(page_dir_entry_t* vm) {
	//TODO:
#ifdef KCONSOLE
	if(_sys_info.fb.v_base != 0) {
		map_pages_size(vm, 
			_sys_info.fb.v_base,
			_sys_info.fb.phy_base,
			_sys_info.fb.size,
			AP_RW_D, PTE_ATTR_DEV);
	}
#endif
}

int32_t  check_mem_map_arch(uint32_t phy_base, uint32_t size) {
    if(phy_base >= _sys_info.fb.phy_base && size <= _sys_info.fb.size)
        return 0;
    if(phy_base >= _sys_info.mmio.phy_base && size <= _sys_info.mmio.size)
        return 0;
    return -1;
}

#if KERNEL_SMP

#define CPUCFG_BASE_ADDR			(MMIO_BASE + 0x1f01c00)

#define CPUCFG_CPU_PWR_CLAMP_STATUS_REG(cpu)	((cpu) * 0x40 + 0x64)
#define CPUCFG_CPU_RST_CTRL_REG(cpu)		(((cpu) + 1) * 0x40)
#define CPUCFG_CPU_CTRL_REG(cpu)		(((cpu) + 1) * 0x40 + 0x04)
#define CPUCFG_CPU_STATUS_REG(cpu)		(((cpu) + 1) * 0x40 + 0x08)
#define CPUCFG_GEN_CTRL_REG			0x184
#define CPUCFG_PRIVATE0_REG			0x1a4
#define CPUCFG_PRIVATE1_REG			0x1a8
#define CPUCFG_DBG_CTL0_REG			0x1e0
#define CPUCFG_DBG_CTL1_REG			0x1e4

#define PRCM_CPU_PWROFF_REG			0x100
#define PRCM_CPU_PWR_CLAMP_REG(cpu)		(((cpu) * 4) + 0x140)

#define PSCI_VERSION	0x84000000
#define PSCI_CPU_ON     0x84000003

extern char __entry[];

struct arm_smccc_res {
    unsigned long a0;
    unsigned long a1;
    unsigned long a2;
    unsigned long a3;
};

extern void smccc_smc(unsigned long a0, unsigned long a1, unsigned long a2,
       unsigned long a3, unsigned long a4, unsigned long a5,
       unsigned long a6, unsigned long a7, struct arm_smccc_res *res);

uint32_t sunxi_smc_call_optee(uint32_t arg0, uint32_t arg1, uint32_t arg2,
        uint32_t arg3, uint32_t pResult)
{
    struct arm_smccc_res param = { 0 };
	uint32_t p = &param;
    arm_smccc_smc(arg0, arg1, arg2, arg3, 0, 0, 0, 0, p);
    return param.a0;
}

void start_core(uint32_t core_id) {
    if(core_id >= _sys_info.cores)
        return;

	//uint32_t ver = sunxi_smc_call_optee(PSCI_VERSION, 0, 0, 0, 0);
	//printf("psci version: %d.%d\n", ver>>16, ver&0xFFFF);
	int ret  =  sunxi_smc_call_optee(PSCI_CPU_ON, core_id, V2P(__entry) , 0, 0);
	printf("\npsci ret: %d\n", ret);
}
#endif

void kalloc_arch(void) {
	kalloc_append(P2V(_allocable_phy_mem_base), P2V(_allocable_phy_mem_top));
}
