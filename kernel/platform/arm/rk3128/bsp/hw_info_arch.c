#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <mm/mmu.h>
#include <kstring.h>

#ifdef KERNEL_SMP
#include <kernel/core.h>
#endif


uint32_t _allocable_phy_mem_top = 0;
uint32_t _allocable_phy_mem_base = 0;



#ifdef KERNEL_SMP
extern char __entry[];

enum pmu_power_domain {
    PD_BCPU,
    PD_BDSP,
    PD_BUS,
    PD_CPU_0,
    PD_CPU_1,
    PD_CPU_2,
    PD_CPU_3,
    PD_CS,
    PD_GPU,
    PD_HEVC,
    PD_PERI,
    PD_SCU,
    PD_VIDEO,
    PD_VIO,
};

#define RK_CRU_VIRT                     (0x20000000)
#define RK_BOOTRAM_VIRT                 (MMIO_BASE + 0x80000)

#define RK312X_CRU_CLKGATES_CON_CNT 11
#define RK312X_CRU_CLKGATE_CON		0xd0
#define RK312X_CRU_CLKGATES_CON(i)	(RK312X_CRU_CLKGATE_CON + ((i) * 4))

#define RK312X_CRU_SOFTRSTS_CON_CNT	(9)
#define RK312X_CRU_SOFTRST_CON		0x110
#define RK312X_CRU_SOFTRSTS_CON(i)	(RK312X_CRU_SOFTRST_CON + ((i) * 4))

#define RK312X_IMEM_VIRT (RK_BOOTRAM_VIRT + 32*KB)

#define writel(v,a) 	(*(volatile uint32_t *)(a) = (v))
#define readl(a) 	(*(volatile uint32_t *)(a))

#define cru_readl(offset)	readl(RK_CRU_VIRT + offset)
#define cru_writel(v, offset)	do { writel(v, RK_CRU_VIRT + offset); dsb(); } while (0)

static inline void dsb(void){
    	__asm("dsb");
}

static inline void dsb_sev(void){
    	__asm("dsb");
    	__asm("sev");
}

static int rk312x_sys_set_power_domain(enum pmu_power_domain pd, uint32_t entry, bool on)
{
    uint32_t clks_save[RK312X_CRU_CLKGATES_CON_CNT];
    uint32_t i, ret = 0;
    for (i = 0; i < RK312X_CRU_CLKGATES_CON_CNT; i++) {
        clks_save[i] = cru_readl(RK312X_CRU_CLKGATES_CON(i));
    }

    for (i = 0; i < RK312X_CRU_CLKGATES_CON_CNT; i++)
        cru_writel(0xffff0000, RK312X_CRU_CLKGATES_CON(i));
    if(on){
        if (pd >= PD_CPU_0 && pd <= PD_CPU_3) {
            writel(0x110000 << (pd - PD_CPU_0),
                    RK_CRU_VIRT + RK312X_CRU_SOFTRSTS_CON(0));
            dsb();
            _delay(100);
            writel(entry, RK312X_IMEM_VIRT + 8);
            writel(0xDEADBEAF, RK312X_IMEM_VIRT + 4);
            dsb_sev();
        }
    }else{
        if (pd >= PD_CPU_0 && pd <= PD_CPU_3) {
            writel(0x10001 << (pd - PD_CPU_0),
                       RK_CRU_VIRT + RK312X_CRU_SOFTRSTS_CON(0));
            dsb();
        }
    }

    for (i = 0; i < RK312X_CRU_CLKGATES_CON_CNT; i++) {
        cru_writel(clks_save[i] | 0xffff0000
            , RK312X_CRU_CLKGATES_CON(i));
    }

    return ret;
}

void __attribute__((optimize("O0"))) prepare_cores(void) {
    for(uint32_t i = 1; i <  _sys_info.cores; i++)
        rk312x_sys_set_power_domain(PD_CPU_0 + i, 0, false); 
}

inline void __attribute__((optimize("O0"))) start_core(uint32_t core_id) { //TODO
	uint32_t entry = V2P((uint32_t)(__entry) + _sys_info.kernel_base);
    if(core_id == 1)
       prepare_cores(); 

    if(core_id < _sys_info.cores)
        rk312x_sys_set_power_domain(PD_CPU_0 + core_id, entry, true);
}
#endif

void sys_info_init_arch(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));
	strcpy(_sys_info.machine, "rk3128");
	strcpy(_sys_info.arch, "armv7");
	_sys_info.phy_offset = 0x60000000;
	_sys_info.vector_base = _sys_info.phy_offset;
	_sys_info.total_phy_mem_size = 256*MB;
    _sys_info.total_usable_mem_size = _sys_info.total_phy_mem_size;
	_sys_info.mmio.phy_base = 0x10000000;
	_sys_info.mmio.size = 8*MB;

	_allocable_phy_mem_base = V2P(get_allocable_start());
	_allocable_phy_mem_top = _sys_info.phy_offset + _sys_info.total_usable_mem_size - 36*MB;

    _sys_info.fb.phy_base = 0x6dd00000;
    _sys_info.fb.v_base = FB_BASE;
    _sys_info.fb.size = 1024*600*4;

#ifdef KERNEL_SMP
	_sys_info.cores = get_cpu_cores();
#else
	_sys_info.cores = 1;
#endif
}

void arch_vm(page_dir_entry_t* vm) {
	//map framebuffer
	map_pages_size(vm, _sys_info.fb.v_base, _sys_info.fb.phy_base, _sys_info.fb.size, AP_RW_D, PTE_ATTR_WRBACK);	
	map_pages_size(vm, _sys_info.mmio.phy_base+0x10000000, _sys_info.mmio.phy_base+0x10000000, _sys_info.mmio.size, AP_RW_D, PTE_ATTR_DEV);	
	map_pages_size(vm, _sys_info.mmio.v_base+0x10000000, _sys_info.mmio.phy_base+0x10000000, _sys_info.mmio.size, AP_RW_D, PTE_ATTR_DEV);	
}

void kalloc_arch(void) {
	kalloc_append(P2V(_allocable_phy_mem_base), P2V(_allocable_phy_mem_top));
}

int32_t  check_mem_map_arch(uint32_t phy_base, uint32_t size) {
	if(phy_base >= _sys_info.fb.phy_base && size <= _sys_info.fb.size)
		return 0;
	if(phy_base >= _sys_info.mmio.phy_base && size <= _sys_info.mmio.size)
		return 0;
	if(phy_base >= (_sys_info.mmio.phy_base+0x10000000) && size <= _sys_info.mmio.size)
		return 0;
	return -1;
}